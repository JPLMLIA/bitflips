# 
#  Filename    : Makefile.am
#  Description : Builds Valgrind extension: BITFLIPS SEU simulator
#  Author(s)   : Ben Bornstein
#
#  $Id: Makefile.am,v 1.1.1.1 2007/05/24 06:11:07 bornstei Exp $
#  $Source: /proj/foamlatte/cvsroot/bitflips/Makefile.am,v $
# 
#  Copyright 2007 California Institute of Technology.  ALL RIGHTS RESERVED.
#  U.S. Government Sponsorship acknowledged.
#


include $(top_srcdir)/Makefile.tool.am

noinst_PROGRAMS = 
if VG_X86_LINUX
noinst_PROGRAMS += bitflips-x86-linux
endif
if VG_AMD64_LINUX
noinst_PROGRAMS += bitflips-amd64-linux
endif
if VG_PPC32_LINUX
noinst_PROGRAMS += bitflips-ppc32-linux
endif
if VG_PPC64_LINUX
noinst_PROGRAMS += bitflips-ppc64-linux
endif

BITFLIPS_SOURCES_COMMON = bf_main.c
SUBDIRS = . 

bitflips_x86_linux_SOURCES      = $(BITFLIPS_SOURCES_COMMON)
bitflips_x86_linux_CPPFLAGS     = $(AM_CPPFLAGS_X86_LINUX)
bitflips_x86_linux_CFLAGS       = $(AM_CFLAGS_X86_LINUX)
bitflips_x86_linux_DEPENDENCIES = $(COREGRIND_LIBS_X86_LINUX)
bitflips_x86_linux_LDADD        = $(TOOL_LDADD_X86_LINUX)
bitflips_x86_linux_LDFLAGS      = $(TOOL_LDFLAGS_X86_LINUX)

bitflips_amd64_linux_SOURCES      = $(BITFLIPS_SOURCES_COMMON)
bitflips_amd64_linux_CPPFLAGS     = $(AM_CPPFLAGS_AMD64_LINUX)
bitflips_amd64_linux_CFLAGS       = $(AM_CFLAGS_AMD64_LINUX)
bitflips_amd64_linux_DEPENDENCIES = $(COREGRIND_LIBS_AMD64_LINUX)
bitflips_amd64_linux_LDADD        = $(TOOL_LDADD_AMD64_LINUX)
bitflips_amd64_linux_LDFLAGS      = $(TOOL_LDFLAGS_AMD64_LINUX)

bitflips_ppc32_linux_SOURCES      = $(BITFLIPS_SOURCES_COMMON)
bitflips_ppc32_linux_CPPFLAGS     = $(AM_CPPFLAGS_PPC32_LINUX)
bitflips_ppc32_linux_CFLAGS       = $(AM_CFLAGS_PPC32_LINUX)
bitflips_ppc32_linux_DEPENDENCIES = $(COREGRIND_LIBS_PPC32_LINUX)
bitflips_ppc32_linux_LDADD        = $(TOOL_LDADD_PPC32_LINUX)
bitflips_ppc32_linux_LDFLAGS      = $(TOOL_LDFLAGS_PPC32_LINUX)

bitflips_ppc64_linux_SOURCES      = $(BITFLIPS_SOURCES_COMMON)
bitflips_ppc64_linux_CPPFLAGS     = $(AM_CPPFLAGS_PPC64_LINUX)
bitflips_ppc64_linux_CFLAGS       = $(AM_CFLAGS_PPC64_LINUX)
bitflips_ppc64_linux_DEPENDENCIES = $(COREGRIND_LIBS_PPC64_LINUX)
bitflips_ppc64_linux_LDADD        = $(TOOL_LDADD_PPC64_LINUX)
bitflips_ppc64_linux_LDFLAGS      = $(TOOL_LDFLAGS_PPC64_LINUX)
