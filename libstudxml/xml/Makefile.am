# file      : xml/Makefile.am
# copyright : Copyright (c) 2013-2014 Code Synthesis Tools CC
# license   : MIT; see accompanying LICENSE file

EXTRA_DIST = details/config-vc.h details/expat/ascii.h details/expat/asciitab.h details/expat/config.h details/expat/iasciitab.h details/expat/internal.h details/expat/latin1tab.h details/expat/nametab.h details/expat/utf8tab.h details/expat/xmlrole.h details/expat/xmltok.h details/expat/xmltok_impl.c details/expat/xmltok_impl.h details/expat/xmltok_ns.c details/expat/README details/expat/LICENSE details/genx/README details/genx/LICENSE libstudxml-vc9.vcproj libstudxml-vc10.vcxproj libstudxml-vc10.vcxproj.filters libstudxml-vc11.vcxproj libstudxml-vc11.vcxproj.filters libstudxml-vc12.vcxproj libstudxml-vc12.vcxproj.filters

studxmlincludedir = $(includedir)/xml

lib_LTLIBRARIES = libstudxml.la
libstudxml_la_SOURCES = qname.cxx parser.cxx serializer.cxx value-traits.cxx

nobase_studxmlinclude_HEADERS = parser.ixx details/post.hxx details/export.hxx details/pre.hxx details/config.hxx serializer.ixx value-traits.txx parser.txx
nobase_nodist_studxmlinclude_HEADERS = details/config.h

if !LIBSTUDXML_EXTERNAL_EXPAT
libstudxml_la_SOURCES += details/expat/xmlparse.c details/expat/xmlrole.c details/expat/xmltok.c
nobase_studxmlinclude_HEADERS += details/expat/expat.h details/expat/expat_external.h
endif

libstudxml_la_SOURCES += details/genx/genx.c details/genx/char-props.c
nobase_studxmlinclude_HEADERS += details/genx/genx.h

nobase_studxmlinclude_HEADERS += content exception forward qname parser serializer value-traits

# Make sure make doesn't try to build the no-extension headers
# thinking that they are executable.
#
content $(top_srcdir)/xml/content exception $(top_srcdir)/xml/exception forward $(top_srcdir)/xml/forward qname $(top_srcdir)/xml/qname parser $(top_srcdir)/xml/parser serializer $(top_srcdir)/xml/serializer value-traits $(top_srcdir)/xml/value-traits : ; @:

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) -DLIBSTUDXML_DYNAMIC_LIB
AM_LDFLAGS = -release 1.0 -no-undefined
