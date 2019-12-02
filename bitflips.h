/** 
 * \file    bitflips.h
 * \brief   Valgrind Tool: BITFLIPS SEU simulator
 * \author  Ben Bornstein
 *
 * $Id: bitflips.h,v 1.1.1.1 2007/05/24 06:11:07 bornstei Exp $
 * $Source: /proj/foamlatte/cvsroot/bitflips/bitflips.h,v $
 */
/* Copyright 2007 California Institute of Technology.  ALL RIGHTS RESERVED.
 * U.S. Government Sponsorship acknowledged.
 */


#ifndef __BITFLIPS_H
#define __BITFLIPS_H


#include "valgrind.h"


/**
 * BITFLIPS makes use of Valgrind's Client Request mechanism so that a
 * running program may indicate which areas (blocks) of memory are
 * allowed to be hit with Single Event Upsets (SEUs).
 *
 * The macros VALGRIND_BITFLIPS_ON() and VALGRIND_BITFLIPS_OFF()
 * control whether or not the fault injection mechanism is enabled.
 */

typedef enum
{
    BITFLIPS_CHAR      =   1
  , BITFLIPS_UCHAR     =   2
  , BITFLIPS_SHORT     =   4
  , BITFLIPS_USHORT    =   8
  , BITFLIPS_INT       =  16
  , BITFLIPS_UINT      =  32
  , BITFLIPS_LONG      =  64
  , BITFLIPS_ULONG     = 128
  , BITFLIPS_FLOAT     = 256
  , BITFLIPS_DOUBLE    = 512
} VgBF_MemType_t;


typedef enum
{
    BITFLIPS_ROW_MAJOR = 1024
  , BITFLIPS_COL_MAJOR = 2048
} VgBF_MemOrder_t;


typedef enum
{
    VG_USERREQ__BITFLIPS_ON = VG_USERREQ_TOOL_BASE('B','F')
  , VG_USERREQ__BITFLIPS_OFF
  , VG_USERREQ__BITFLIPS_MEM_ON
  , VG_USERREQ__BITFLIPS_MEM_OFF
} VgBF_ClientRequest_t;


#define VALGRIND_BITFLIPS_ON()                \
  (__extension__({unsigned int _qzz_res;      \
   VALGRIND_DO_CLIENT_REQUEST(_qzz_res, 0, VG_USERREQ__BITFLIPS_ON, \
                              0, 0, 0, 0, 0); \
   _qzz_res;                                  \
   }))


#define VALGRIND_BITFLIPS_OFF()               \
  (__extension__({unsigned int _qzz_res;      \
   VALGRIND_DO_CLIENT_REQUEST(_qzz_res, 0, VG_USERREQ__BITFLIPS_OFF, \
                              0, 0, 0, 0, 0); \
     _qzz_res;                                \
   }))


#define VALGRIND_BITFLIPS_MEM_ON(addr, nrows, ncols, type, order)        \
  (__extension__({unsigned int _qzz_res;                                 \
   VALGRIND_DO_CLIENT_REQUEST(_qzz_res, 0, VG_USERREQ__BITFLIPS_MEM_ON,  \
                              addr, nrows, ncols, #addr, type | order);  \
     _qzz_res;                                                           \
   }))


#define VALGRIND_BITFLIPS_MEM_OFF(addr)                                  \
  (__extension__({unsigned int _qzz_res;                                 \
   VALGRIND_DO_CLIENT_REQUEST(_qzz_res, 0, VG_USERREQ__BITFLIPS_MEM_OFF, \
                              addr, 0, 0, #addr, 0);                     \
     _qzz_res;                                                           \
   }))


#endif  /* __BITFLIPS_H */
