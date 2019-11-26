#!/usr/bin/env python


##
##   usage: bitflips [options] <program>
## options:
##   --fault-probability=<float>  [0.0 1.0]  (default: 0.05)
##   --fault-rate=<float>         [0.0 1.0]  (default: 0)
##   --inject-faults=yes|no                  (default: yes)
##   --seed=<int>                            (default: 42)
##   --verbose=yes|no                        (default: no)
##
## Runs the Valgrind BITFLIPS tool on program.
##
## (NOTE: This wrapper program is necessary to cope with Valgrind's
## lack of standard libc floating-point I/O, performing the necessary
## translations to and from unsigned ints.)
##
## Author: Ben Bornstein
##

#
# $Id: bitflips,v 1.1 2007/05/24 06:17:24 bornstei Exp $
# $Source: /proj/foamlatte/cvsroot/bitflips/bitflips,v $
#
# Copyright 2007 California Institute of Technology.  ALL RIGHTS RESERVED.
# U.S. Government Sponsorship acknowledged.
#


import os
import struct
import sys


def usage ():
  """usage()

  Prints the usage statement at the top of this program.
  """
  stream = open(sys.argv[0])
  for line in stream.readlines():
    if line.startswith('##'): print line.replace('##', ''),
  stream.close()


def reinterpret_cast (value, from_format, to_format):
  """reinterpret_cast(value, from_format, to_format) -> value

  Casts the Python type of value from from_format to to_format (the
  underlying bit representation is preserved).  Format specifiers come
  from the Python 'struct' package:

    http://docs.python.org/lib/module-struct.html
  
  and from_format and to_format must be the same size in bits, i.e.:

    struct.calsize(from_format) == struct.calcsize(to_format)
  """
  return struct.unpack(to_format, struct.pack(from_format, value))[0]


def float2int (s):
  """float2int(s) -> string

  Converts the given floating-point string to an integer whose bits
  represent the floating-point value and returns it.  Platform
  endianness is assumed.
  """
  return str( reinterpret_cast(float(s), "f", "i") )


def hex2float (s):
  """hex2float(s) -> string

  Converts the given hexadecimal string to a floating-point string and
  returns it.  The precision (single or double) of the floating-point
  number is determined by the number of digits in the hex string (8 or
  16, respectively).  Platform endianness is assumed.
  """
  if len(s) == 8:
    from_format = "I"
    to_format   = "f"
  else:
    from_format = "Q"
    to_format   = "d"

  return str( reinterpret_cast(int(s, 16), from_format, to_format) )


if len(sys.argv) < 2:
  usage()
  sys.exit(2)

args = [ ]

for arg in sys.argv[1:]:
  if arg.startswith("--fault-probability=") or arg.startswith("--fault-rate="):
    original  = arg.split("=")[1]
    converted = float2int(original)
    arg       = arg.replace(original, converted)
  args.append(arg)

command = "valgrind --tool=bitflips " + " ".join(args)
print command

stream = os.popen4(command)[1]

for line in stream.readlines():
  if line.count("BF:") == 1:
    tokens = line.split()
    orig   = tokens[5]
    flip   = tokens[9]
    line   = line.replace(orig, hex2float(orig))
    line   = line.replace(flip, hex2float(flip))
  elif line.count("fault-probability:") == 1 or line.count("fault-rate:") == 1:
    tokens = line.split(":")
    orig   = tokens[1].strip()
    line   = line.replace(orig, hex2float(orig))
  print line,

stream.close()
