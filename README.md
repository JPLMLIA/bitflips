# BITFLPS: Basic Instrumentation Tool for Fault Localized Injection of Probabilistic SEUs
Version 1.0.0

BITFLIPS is a valgrind extension to simulate radiation-induced
bitflips to user-specified exposed memory.

Author: Ben Bornstein

With contributions from Rob Granat and Kiri Wagstaff

Contact: ben.bornstein@jpl.nasa.gov

BITFLIPS JPL NTR: #45369


# Quick Start

In the JPL Machine Learning group, on analysis (or another 64-bit MLIA
machine):

```Console
$ source /proj/foamlatte/setup.sh   # (for (ba)sh), or
$ source /proj/foamlatte/setup.csh  # (for (t)csh)
```

Run your program under the BITFLIPS tool extension to Valgrind using
the Python wrapper, e.g.:

```Console
$ ./bitflips.py --seed=42 --fault-probability=5 /proj/foamlatte/code/bitflips/test/dotprodd
```

The Python wrapper "bitflips" is located in `/proj/foamlatte/tps/bin`,
which is added to your PATH by the FOAM LATTE setup scripts.

The `dotprodd` (double) and `dotprodf` (float) example programs perform a 
dot product on a 1000-element double or float vector, respectively.
The dot product is computed twice, once with SEU fault injection off
and then again with it on.  The `dotprodd.c` and `dotprodf.c` C programs
demonstrate how to communicate with the BITFLIPS engine via
`VALGRIND_BITFLIPS` macros.  The macros result in processor no-ops when
your program is run standalone (outside of BITFLIPS), so it's
unobtrusive, convenient, and safe to leave them in your source code
and compiled program at all times.


# Introduction

BITFLIPS is software simulator for injecting single event upsets
(SEUs) into a running computer program.  The software is written as a
plugin extension module of the open source
[Valgrind](http://valgrind.org/) debugging and profiling tool.

BITFLIPS can inject SEUs into any program which can be run on the
Linux operating system, without modifying the original program or
requiring access to the program source code.  If access to the
original program source code is available, BITFLIPS can offer
fine-grained control over exactly when and which areas of memory
(program variables) may be subjected to SEUs.  SEU injection rate is
controlled by specifying either a fault probability or a fault rate
based on memory size and radiation exposure time in terms of SEUs per
byte per second.  BITFLIPS also has the capability to log each SEU it
injects and, if program source code is available, report the magnitude
of the SEU on the program variable (i.e. did an SEU change a
floating-point value by a small or large amount?).


# Requirements

BITFLIPS can run on any desktop class PC.  Its only requirement is the
Linux operating system.  This limitation results from its dependence
on Valgrind.

In the JPL Machine Learning group any 64-bit Linux machine will do,
e.g. analysis, paralysis, mlia-compute1, or mlia-compute2.


# Build and Installation

While general build instructions are included, if you have access to
the JPL Machine Learning machines and the FOAM LATTE project
directories (under `/proj/foamlatte`), you may want to skip below to
"FOAM LATTE Specific Instructions."


## General Instructions

BITFLIPS is a Valgrind extension and therefore cannot be compiled or
run without Valgrind.  First, download and untar a recent version of
Valgrind from [http://valgrind.org/] (BITFLIPS has been tested with
Valgrind 3.2.3):

```Console
$ tar jxvf valgrind-3.2.3.tar.bz2
```

Checkout BITFLIPS from CVS into the Valgrind source directory:

```Console
$ cd valgrind-3.2.3
$ cvs -d /proj/foamlatte/cvsroot co bitflips
```

Edit Valgrind's `configure.in` and `Makefile.am` to inform it of the
presence of the BITFLIPS module.  In `configure.in`, add
"bitflips/Makefile" to the end of the (long) list of Makefiles in the
AC_OUTPUT directive, changing from:

```
  AC_OUTPUT(
     Makefile 
     valgrind.spec
     ...
     none/tests/x86/Makefile
     none/docs/Makefile
  )
```

to:

```
  AC_OUTPUT(
     Makefile 
     valgrind.spec
     ...
     none/tests/x86/Makefile
     none/docs/Makefile
     bitflips/Makefile
  )
```

In `Makefile.am`, add "bitflips" to the list of TOOLS, changing from:

```
  TOOLS =         memcheck \
                  cachegrind \
                  callgrind \
                  massif \
                  lackey \
                  none
```

to:

```
  TOOLS =         memcheck \
                  cachegrind \
                  callgrind \
                  massif \
                  lackey \
                  none \
                  bitflips
```

Then simple follow the standard Valgrind build and installation
instructions.  In brief:

```Console
$ ./configure
$ make
$ make install
```


## FOAM LATTE Specific Instructions

The FOAM LATTE project directory (`/proj/foamlatte`) uses Stow package
management system to maintain and switch between several versions of
Valgrind / BITFLIPS.  This section describes the easiest way to clone,
modify, and then "install" from an existing installation.

Clone the most recent Valgrind / BITFLIPS source tree, e.g.:

```Console
$ cd /proj/foamlatte/tps/src
$ cp -pr valgrind-3.2.3-bitflips-0.2.0 valgrind-3.2.3-bitflips-<version>
```

Where <version> is the new version number for the BITFLIPS
installation you will be creating.  Rerun ./configure with an updated
--prefix to reflect the new BITFLIPS version number:

```Console
$ cd valgrind-3.2.3-bitflips-<version>
$ ./configure --prefix=/proj/foamlatte/tps/stow/valgrind-3.2.3-bitflips-<version>
```

Next, ensure BITFLIPS contains the latest CVS updates:

```Console
$ cd bitflips
$ cvs update -d -P
```

and apply your source code updates, including updating the VERSION.txt
file and changing the BITFLIPS bf_main.c source code to reflect the
new version number.  To update the version number in bf_main.c, change
the version string in passed to the VG_(details_version) macro (toward
the end of the .c file) from something like:

```
  VG_(details_version)("0.2.0");
```

to something like:

```
  VG_(details_version)("0.3.0");
```

Save your changes.

To build Valgrind / BITFLIPS, cd to the root of the Valgrind source
tree and make:

```Console
$ cd valgrind-3.2.3-bitflips-<version>/
$ make
```

To run BITFLIPS in order to test out your changes (before installing
to `/proj/foamlatte/tps/bin` for everyone to use):

```Console
$ export VALGRIND_LIB=`pwd`/.in_place  # (ba)sh
$ setenv VALGRIND_LIB `pwd`/.in_place  # (t)csh
```

Where `pwd` should expand to valgrind-3.2.3-bitflips-<version>/.  See
Valgrind's README_DEVELOPERS for more information.

Then run:

```Console
$ coregrind/valgrind --tool=bitflips ...
```

If you would like to use test with the BITFLIPS Python wrapper, make a
local copy:

```Console
$ cp -p /proj/foamlatte/tps/bin/bitflips .
```

and change the word "valgrind" in the line:

```
  command = "valgrind --tool=bitflips " + " ".join(args)
```

to "coregrind/valgrind":

```
  command = "coregrind/valgrind --tool=bitflips " + " ".join(args)
```

Finally, to install this new version of Valgrind / BITFLIPS to
`/proj/foamlatte/tps`:

```Console
$ make install
$ cd /proj/foamlatte/tps/stow
$ stow -D valgrind-3.2.3-bitflips-0.2.0  # (uninstall v 0.2.0)
$ stow    valgrind-3.2.3-bitflips-0.3.0  # (install   v 0.3.0)
```

To see which version of Valgrind / BITFLIPS is currently installed:

```Console
$ ls -l /proj/foamlatte/tps/bin/valgrind
  lrwxrwxrwx  1 bornstei users 50 Jun 13 14:35 /proj/foamlatte/tps/bin/valgrind -> ../stow/valgrind-3.2.3-bitflips-0.2.0/bin/valgrind
```

# Command-line Parameters

The command-line parameters described below are for the BITFLIPS
Python wrapper program.

```
  --fault-probability=<float>  [0.0 1.0]  (default: 0.05)

    Indicates the probability of triggering a fault on each CPU
    instruction.

  --fault-rate=<float>  [0.0 1.0]  (default: 0)

    This parameter trumps --fault-probability (causing it to be
    ignored.)  The fault rate specifies the number of SEUs that should
    occur per byte per instruction.  The actual fault rate achieved
    (<= to the fault rate specified) is output when BITFLIPS
    terminates.

  --inject-faults=yes|no  (default: yes)

    This parameter sets the initial state of the fault injector to be
    either on or off.  This is equivalent to putting one of the
    following BITFLIPS directives at the start of your program:

      VALGRIND_BITFLIPS_ON();
      VALGRIND_BITFLIPS_OFF();

  --seed=<int>  (default: 42)

    This parameter is used to control the generation of SEU events and
    allows the results of a particular run to be reproduced.

  --verbose=yes|no

    As the name implies, this parameter controls whether or not
    copious (debug) output is generated.
```

#  Program Macros

The macros described in this section result in processor no-ops when
your program is run standalone (outside of BITFLIPS), so it's
unobtrusive, convenient, and safe to leave them in your source code
and compiled program at all times.

To turn fault injection on and off in your code:

```
  /* compile with gcc -I/proj/foamlatte/tps/include */
  #include <valgrind/bitflips.h>
```

Use the macros `VALGRIND_BITFLIPS_ON();` and `VALGRIND_BITFLIPS_OFF();`

To declare which blocks of memory may receive faults and when, use:

```
  VALGRIND_BITFLIPS_MEM_ON(baseaddr, nrows, ncols, type, layout);
  VALGRIND_BITFLIPS_MEM_OFF(baseaddr);
```

Where type is one of:

```
  BITFLIPS_CHAR
  BITFLIPS_UCHAR
  BITFLIPS_SHORT
  BITFLIPS_USHORT
  BITFLIPS_INT
  BITFLIPS_UINT
  BITFLIPS_LONG
  BITFLIPS_ULONG
  BITFLIPS_FLOAT
  BITFLIPS_DOUBLE
```

And layout is one of:

```
  BITFLIPS_ROW_MAJOR
  BITFLIPS_COL_MAJOR
```

For example:

```
  VALGRIND_BITFLIPS_MEM_ON(&v1[0], size, 1, BITFLIPS_DOUBLE, BITFLIPS_ROW_MAJOR);
  /* Compute dot product of v1 and ... */
  VALGRIND_BITFLIPS_MEM_OFF(&v1[0]);
```

See the example dot-product program and Makefile in:

```
  /proj/foamlatte/code/bitflips/test/
```


NOTE: The BITFLIPS `MEM_ON` and `MEM_OFF` macro parameters require the
number of rows and columns, data type and memory layout of the program
variables.  This additional information greatly improves the quality
of BITFLIPS SEU trace log output.  Because of its numerical
algorithm-based fault tolerance (ABFT) roots, BITFLIPS is biased
toward areas of contiguous memory that store N-dimensional vectors,
and M-by-N matrices of floats and doubles.  For memory containing
scalar values, simply set both the number of rows and columns to one
(1) and use a layout of `BITFLIPS_ROW_MAJOR` (C's default memory
layout).


