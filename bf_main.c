/** 
 * \file    bf_main.c
 * \brief   Valgrind Tool: BITFLIPS SEU simulator
 * \author  Ben Bornstein
 *
 * $Id: bf_main.c,v 1.2 2007/09/18 05:29:28 bornstei Exp $
 * $Source: /proj/foamlatte/cvsroot/bitflips/bf_main.c,v $
 */
/* Copyright 2007 California Institute of Technology.  ALL RIGHTS RESERVED.
 * U.S. Government Sponsorship acknowledged.
 */

#include "pub_tool_basics.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_execontext.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_options.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_machine.h"
#include "pub_tool_mallocfree.h"

#include "VEX/priv/main_util.h"
#include "VEX/pub/libvex_guest_x86.h"

#include "bitflips.h"


#define BF_(str)    VGAPPEND(vgBitFlips_,str)


typedef struct _VgBF_MemBlock_t
{
  Addr              start;
  Addr              end;
  SizeT             num_bytes;
  SizeT             num_rows;
  SizeT             num_cols;
  SizeT             num_elems;
  double            num_kilobytes;
  double            num_faults_pending;
  HChar*            desc;
  VgBF_MemType_t    type;
  VgBF_MemOrder_t   layout;
  ExeContext*       where;

  struct _VgBF_MemBlock_t* next;

} VgBF_MemBlock_t;


typedef struct
{ 
  UInt  bits;
  UInt  lo;
  UInt  hi;
}
VgBF_Distribution_t;


/**
 * The following probability distribution indicates how often a certain
 * number of bits will be flipped.
 */
VgBF_Distribution_t BitFlipDensity[] =
{
    {.bits = 1, .lo =  0, .hi = 60}
  , {.bits = 2, .lo = 61, .hi = 90}
  , {.bits = 3, .lo = 91, .hi = 95}
  , {.bits = 4, .lo = 96, .hi = 97}
  , {.bits = 5, .lo = 97, .hi = 97}
  , {.bits = 6, .lo = 98, .hi = 98}
  , {.bits = 7, .lo = 99, .hi = 99}
};

/*                                1111111111222222222233 */       
/*                       1234567890123456789012345678901 */
static UInt              FaultCount       = 0;
static float             FaultRate        = 0;
static Bool              FaultInjection   = True;
static ULong             InstructionCount = 0;
static double            KilobyteFlux     = 0.0;
static UInt              Seed             = 42;
static Bool              Verbose          = False;
static VgBF_MemBlock_t*  MemBlockHead     = 0;


/**
 * @return a uniform random integer number in the range [0 (n - 1)].
 */
static UInt
BF_(randomInt) (UInt* seed, UInt n)
{
  return VG_(random)(seed) % n;
}


/**
 * @return the number of bytes of storage required for the given
 * VgBF_MemType.
 */
static UInt
BF_(sizeof) (VgBF_MemType_t type)
{
  UInt bytes = 0;


  switch (type)
  {
    case BITFLIPS_CHAR:
    case BITFLIPS_UCHAR:
      bytes = sizeof(char);
      break;

    case BITFLIPS_SHORT:
    case BITFLIPS_USHORT:
      bytes = sizeof(short);
      break;

    case BITFLIPS_INT:
    case BITFLIPS_UINT:
      bytes = sizeof(int);
      break;

    case BITFLIPS_LONG:
    case BITFLIPS_ULONG:
      bytes = sizeof(long);
      break;

    case BITFLIPS_FLOAT:
      bytes = sizeof(float);
      break;

    case BITFLIPS_DOUBLE:
      bytes = sizeof(double);
      break;

    default:
      bytes = 0;
      break;
  }

  return bytes;
}


/**
 * @return the column corresponding to the given address, for the
 * matrix stored at MemBlock block.
 */
static UInt
BF_(MemBlock_getCol) (VgBF_MemBlock_t* block, Addr addr)
{
  UInt offset = (addr - block->start) / BF_(sizeof)(block->type);

  return (block->layout == BITFLIPS_COL_MAJOR) ?
    offset / block->num_rows :
    offset % block->num_rows;
}


/**
 * @return the row corresponding to the given address, for the matrix
 * stored at MemBlock block.
 */
static UInt
BF_(MemBlock_getRow) (VgBF_MemBlock_t* block, Addr addr)
{
  UInt offset = (addr - block->start) / BF_(sizeof)(block->type);

  return (block->layout == BITFLIPS_ROW_MAJOR) ?
    offset / block->num_cols :
    offset % block->num_cols;
}


/**
 * Marks the memory passed via the Valgrind Client Request mechanism
 * as susceptible to SEUs.
 */
static void
BF_(MemOn) (ThreadId tid, UWord* arg)
{
  UInt             bytes;
  VgBF_MemBlock_t* block = VG_(malloc)( "bf", sizeof(VgBF_MemBlock_t) );


  block->start     = arg[1];
  block->num_rows  = arg[2];
  block->num_cols  = arg[3];
  block->desc      = VG_(strdup)( "bf", (HChar *) arg[4] );
  block->type      = arg[5] & (BITFLIPS_ROW_MAJOR - 1);
  block->layout    = arg[5] & (BITFLIPS_ROW_MAJOR + BITFLIPS_COL_MAJOR);
  block->where     = VG_(record_ExeContext)(tid, 0);
  block->next      = 0;


  bytes                = BF_(sizeof)(block->type);
  block->num_elems     = block->num_rows  * block->num_cols;
  block->num_bytes     = block->num_elems * bytes;
  block->num_kilobytes = block->num_bytes / 1000.0;
  block->end           = block->start + block->num_bytes - 1;

  block->num_faults_pending = 0.0;

  if (MemBlockHead == 0)
  {
    MemBlockHead = block;
  }
  else
  {
    block->next  = MemBlockHead;
    MemBlockHead = block;
  }
}


/**
 * Marks the memory passed via the Valgrind Client Request mechanism
 * as immune to SEUs.
 */
static void
BF_(MemOff) (UWord* arg)
{
  VgBF_MemBlock_t* block = MemBlockHead;
  VgBF_MemBlock_t* prev  = 0;


  while (block != 0)
  {
    if (block->start == arg[1])
    {
      if (prev != 0)
      {
        prev->next = block->next;
      }
      else
      {
        MemBlockHead = block->next;
      }

      VG_(free)(block->desc);
      VG_(free)(block);
      break;
    }

    prev  = block;
    block = block->next;
  }
}


#if 0
/**
 * @return the SEU susceptible MemBlock that contains address or null
 * (0) if no such block can be found.
 */
static VgBF_MemBlock_t*
BF_(Addr_getMemBlock) (Addr addr)
{
  VgBF_MemBlock_t* block = MemBlockHead;


  while (block != 0)
  {
    /*
    VG_(printf)(  "addr: %08p, [start: %08p, end: %08p]\n"
                , addr, block->start, block->end);
    */
    if (addr >= block->start && addr <= block->end)
    {
      return block;
    }

    block = block->next;
  }

  return 0;
}
#endif  /* #if 0 */


/**
 * @return a bit flip mask width bits wide with flips bits flipped.
 */
static UWord
BF_(getFlipMask) (UInt width, UInt flips)
{
    if (flips == 1) {
        // Special case for likely case of only one flip: just shift 1 into a
        // random position
        return (1UL << (VG_(random)(&Seed) % width));
    }

    // Otherwise, start with a mask that has `flips` bits set to 1 in the LSB
    UWord mask = ~((~0UL) << flips);

    // Fischer-Yates Shuffle to shuffle the flipped bits
    // Bit Swap:
    // https://www.geeksforgeeks.org/how-to-swap-two-bits-in-a-given-integer/
    UWord xor;
    UChar i, swap, bit1, bit2;
    for (i = width - 1; i > 0; i--) {
        swap = VG_(random)(&Seed) % (i + 1);
        bit1 = (mask >> swap) & 1;
        bit2 = (mask >> i) & 1;
        xor = bit1 ^ bit2;
        mask ^= ((xor << swap) | (xor << i));
    }
    return mask;
}


/**
 * @return the number of bits to flip based on the BitFlipDensity.
 */
static UInt
BF_(getFlipSize) (void)
{
  UInt n;
  UInt bits = 0;
  UInt rand = VG_(random)(&Seed) % 100;
  UInt size = sizeof(BitFlipDensity) / sizeof(BitFlipDensity[0]);


  for (n = 0; n < size; ++n)
  {
    if (rand >= BitFlipDensity[n].lo && rand <= BitFlipDensity[n].hi)
    {
      bits = BitFlipDensity[n].bits;
      break;
    }
  }

 return bits;
}


/**
 * Flips from 1--8 bits (governed by BF_(getFlipSize)() and the
 * BitFlipDensity PDF) at the given address (size bytes wide).  The
 * MemBlock containing addr is passed-in for reporting purposes.
 */
static void
BF_(doFlipBits) (Addr addr, SizeT size, VgBF_MemBlock_t* block)
{
  UInt row = BF_(MemBlock_getRow)(block, addr);
  UInt col = BF_(MemBlock_getCol)(block, addr);

  if (size == 1)
  {
    UChar  mask     = BF_(getFlipMask)( 8, BF_(getFlipSize)() );
    UChar* p        = (UChar*) addr;
    UChar  original = *p;
    UChar  flipped  = (original ^ mask);

    *p = flipped;
    FaultCount++;

    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "BF: %s %d %d %d %02x %02x %02x\n",
                   block->desc, block->type, row, col, original, mask, flipped);
    }
  }
  else if (size == 2)
  {
    UShort  mask     = BF_(getFlipMask)( 16, BF_(getFlipSize)() );
    UShort* p        = (UShort*) addr;
    UShort  original = *p;
    UShort  flipped  = (original ^ mask);

    *p = flipped;
    FaultCount++;

    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "BF: %s %d %d %d %04x %04x %04x\n",
                   block->desc, block->type, row, col, original, mask, flipped);
    }
  }
  else if (size == 4)
  {
    UInt  mask     = BF_(getFlipMask)( 32, BF_(getFlipSize)() );
    UInt* p        = (UInt*) addr;
    UInt  original = *p;
    UInt  flipped  = (original ^ mask);

    *p = flipped;
    FaultCount++;

    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "BF: %s %d %d %d %08x %08x %08x\n",
                   block->desc, block->type, row, col, original, mask, flipped);
    }
  }
  else if (size == 8)
  {
    UWord  mask     = BF_(getFlipMask)( 64, BF_(getFlipSize)() );
    UWord* p        = (UWord*) addr;
    UWord  original = *p;
    UWord  flipped  = (original ^ mask);

    *p = flipped;
    FaultCount++;

    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "BF: %s %d %d %d %016lx %016lx %016lx\n",
                   block->desc, block->type, row, col, original, mask, flipped);
    }
  }
}


/**
 * If FaultInjection is True, inject approximately FaultRate SEUs / (KB * s)
 * across eligible memory blocks.
 *
 * This function is instrumented (called) in the user's program before
 * every instruction.
 */
static void
BF_(doFaultCheck) (void)
{
  ++InstructionCount;

  if (FaultInjection == True)
  {
    VgBF_MemBlock_t* block = MemBlockHead;

    while (block != 0)
    {
      double pending      = block->num_faults_pending;
      double fraction     = block->num_kilobytes;
      double total_faults = (FaultRate * fraction) + pending;
      UInt   whole_faults = (UInt) total_faults;
      UInt   size         = BF_(sizeof)(block->type);

      block->num_faults_pending  = total_faults - whole_faults;
      KilobyteFlux              += block->num_kilobytes;

      while (whole_faults > 0)
      {
        UInt n    = BF_(randomInt)(&Seed, block->num_elems);
        Addr addr = block->start + (n * size);

        BF_(doFlipBits)(addr, size, block);

        --whole_faults;
      }

      block = block->next;
    }
  }
}


#if 0
/**
 * This function is instrumented (called) in the user's program before
 * every load instruction.
 */
static VG_REGPARM(2) void
BF_(doLoad) (Addr addr, SizeT size)
{
  VG_(printf)("Exec: BF_(doLoad): LOAD: %p (size=%d)\n", addr, size);
}
#endif  /* #if 0 */


/* ------------------------------------------------------------ */
/* -- Instrumentation Functions                              -- */
/* ------------------------------------------------------------ */


static void
BF_(addFaultCheck) (IRSB* bb)
{
  IRExpr** argv = mkIRExprVec_0();
  IRDirty* di   = unsafeIRDirty_0_N(  0
                                    , "BF_(doFaultCheck)" 
				    , VG_(fnptr_to_fnentry)(&BF_(doFaultCheck))
                                    , argv );

  addStmtToIRSB(bb, IRStmt_Dirty(di));
}


#if 0
static void
BF_(addLoadCheck) (IRSB* bb, IRExpr* dAddr, Int dSize)
{
  IRExpr** argv = mkIRExprVec_2(dAddr, mkIRExpr_HWord(dSize));
  IRDirty* di   = unsafeIRDirty_0_N(  2
                                    , "BF_(doLoad)"
                                    , VG_(fnptr_to_fnentry)(&BF_(doLoad))
                                    , argv );

  /*
  if (dAddr->tag == Iex_Const)
  {
    IRConst* constant = (IRConst*) dAddr->Iex.Const.con;
    ULong    address  = (ULong)    constant->Ico.U64;
    VG_(printf)("\nInst: BF_(onLoad): dAddr=0x%llx\n", address);
  }
  */

  addStmtToIRSB(bb, IRStmt_Dirty(di));
}
#endif  /* #if 0 */


/**
 * Main instrumentation function
 */
static IRSB*
BF_(instrument) (  VgCallbackClosure* closure
                 , IRSB*              bbIn
                 , const VexGuestLayout*    layout
                 , const VexGuestExtents*   vge
                 , const VexArchInfo*       archinfo_host
                 , IRType             gWordTy
                 , IRType             hWordTy )
{
  Int n;

  IRSB* bbOut      = emptyIRSB();
  bbOut->tyenv     = deepCopyIRTypeEnv(bbIn->tyenv);
  bbOut->next      = deepCopyIRExpr(bbIn->next);
  bbOut->jumpkind  = bbIn->jumpkind;
  bbOut->offsIP    = bbIn->offsIP;
  

  for (n = 0; n < bbIn->stmts_used; ++n)
  {
    IRStmt* statement = bbIn->stmts[n];

    if (!statement || statement->tag == Ist_NoOp) continue;

    BF_(addFaultCheck)(bbOut);

    /*
    if (statement->tag == Ist_Tmp)
    {
      IRExpr* data = statement->Ist.Tmp.data;
      if (data->tag == Iex_Load)
      {
        Int size = sizeofIRType(data->Iex.Load.ty);
        BF_(addLoadCheck)(bbOut, data->Iex.Load.addr, size);
      }
    }
    */

    addStmtToIRSB(bbOut, statement);
  }

  return bbOut;
}



/*------------------------------------------------------------*/
/*--- Handle client requests                                --*/
/*------------------------------------------------------------*/


static Bool
BF_(handle_client_request) (ThreadId tid, UWord* arg, UWord *ret)
{
  if (!VG_IS_TOOL_USERREQ('B','F', arg[0]))
  {
    return False;
  }

  switch(arg[0])
  {
    case VG_USERREQ__BITFLIPS_ON:
      if (Verbose)
      {
        VG_(message)(Vg_UserMsg, "VALGRIND_BITFLIPS_ON\n");
      }
      FaultInjection = True;
      *ret           = 0;
      break;

    case VG_USERREQ__BITFLIPS_OFF:
      if (Verbose)
      {
        VG_(message)(Vg_UserMsg, "VALGRIND_BITFLIPS_OFF\n");
      }
      FaultInjection = False;
      *ret           = 0;
      break;

  case VG_USERREQ__BITFLIPS_MEM_ON:
    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "VALGRIND_BITFLIPS_MEM_ON:  %s\n", (char*)arg[4]);
    }
    BF_(MemOn)(tid, arg);
    *ret = 0;
    break;

  case VG_USERREQ__BITFLIPS_MEM_OFF:
    if (Verbose)
    {
      VG_(message)(Vg_UserMsg, "VALGRIND_BITFLIPS_MEM_OFF: %s\n", (char*)arg[4]);
    }
    BF_(MemOff)(arg);
    *ret = 0;
    break;

    default:
      return False;
  }

  return True;
}



/*------------------------------------------------------------*/
/*-- Command-line, usage, initialization, finalization        */
/*------------------------------------------------------------*/


static Bool
BF_(command_line_options) (const HChar* arg)
{
  UInt  rate = 0;


  if      VG_INT_CLO (arg, "--fault-rate"   , rate           ) {}
  else if VG_BOOL_CLO(arg, "--inject-faults", FaultInjection ) {}
  else if VG_INT_CLO (arg, "--seed"         , Seed           ) {}
  else if VG_BOOL_CLO(arg, "--verbose"      , Verbose        ) {}
  else {
    return False;
  }

  if (0 == VG_(strncmp)(arg, "--fault-rate", 12))
  {
    UInt* p = (UInt*) &FaultRate;
    VG_(memcpy)(p, &rate, 4);
  }

  return True;
}


static void
BF_(usage) (void)
{
   VG_(printf)
   ( 
     "    --fault-rate=<int>      (units: faults per KB * sec)\n"
     "    --inject-faults=yes|no  (default: yes)\n"
     "    --seed=<int>            (default: 42)\n"
     "    --verbose=yes|no        (default: no)\n\n"
   );
}


static void
BF_(usage_debug) (void)
{

}


static void
BF_(finalize) (Int exitcode)
{
  float rate   = FaultCount / KilobyteFlux;
  UInt* rate_p = (UInt*) &rate;

  VG_(message)(Vg_UserMsg,
         "---------------------------------------------------------\n");
  VG_(message)(Vg_UserMsg, "Total Bit Flips: %d\n", FaultCount);
  VG_(message)(Vg_UserMsg, "Total Instructions: %lu\n",
               (long unsigned int)InstructionCount);
  VG_(message)(Vg_UserMsg, "Fault Rate: %08x\n", *rate_p);
  VG_(message)(Vg_UserMsg,
         "---------------------------------------------------------\n");
}


static void
BF_(post_clo_init) (void)
{  
  UInt*       rate    = (UInt*) &FaultRate;
  const char* inject  = FaultInjection ? "yes" : "no";
  const char* verbose = Verbose        ? "yes" : "no";


  VG_(message)(Vg_UserMsg, "fault-rate: %08x\n" , *rate   );
  VG_(message)(Vg_UserMsg, "inject-faults: %s\n", inject  );
  VG_(message)(Vg_UserMsg, "seed: %d\n"         , Seed    );
  VG_(message)(Vg_UserMsg, "verbose: %s\n"      , verbose );
}


static void
BF_(pre_clo_init) (void)
{
  VG_(details_name)            ("BITFLIPS");
  VG_(details_version)         ("2.0.0");
  VG_(details_description)     ("Injects SEUs into a running program");
  VG_(details_copyright_author)("Ben Bornstein and Kiri Wagstaff");
  VG_(details_bug_reports_to)  ("ben.bornstein@jpl.nasa.gov");

  VG_(basic_tool_funcs)
  (
     BF_(post_clo_init)
   , BF_(instrument)
   , BF_(finalize)
  );

  VG_(needs_command_line_options)
  (
     BF_(command_line_options)
   , BF_(usage)
   , BF_(usage_debug)
  );

  VG_(needs_client_requests)( BF_(handle_client_request) );
}


VG_DETERMINE_INTERFACE_VERSION( BF_(pre_clo_init) )
