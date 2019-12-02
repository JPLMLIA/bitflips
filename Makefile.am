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

#EXTRA_DIST = docs/nl-manual.xml

#----------------------------------------------------------------------------
# bitflips-<platform>
#----------------------------------------------------------------------------

pkginclude_HEADERS = bitflips.h

noinst_PROGRAMS  = bitflips-@VGCONF_ARCH_PRI@-@VGCONF_OS@
if VGCONF_HAVE_PLATFORM_SEC
noinst_PROGRAMS += bitflips-@VGCONF_ARCH_SEC@-@VGCONF_OS@
endif

BITFLIPS_SOURCES_COMMON = bf_main.c

bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_SOURCES      = \
	$(BITFLIPS_SOURCES_COMMON)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS       = $(LTO_CFLAGS) \
	$(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_PRI_CAPS@)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_PRI_CAPS@)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_PRI@ \
	$(LINK) \
	$(bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS) \
	$(bitflips_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS)

if VGCONF_HAVE_PLATFORM_SEC
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_SOURCES      = \
	$(BITFLIPS_SOURCES_COMMON)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS       = $(LTO_CFLAGS) \
	$(AM_CFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_SEC_CAPS@)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_SEC_CAPS@)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_SEC@ \
	$(LINK) \
	$(bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS) \
	$(bitflips_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS)
endif


