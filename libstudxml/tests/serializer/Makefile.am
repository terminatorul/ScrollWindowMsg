# file      : tests/template/Makefile.am
# copyright : not copyrighted - public domain

EXTRA_DIST = serializer-vc9.vcproj serializer-vc10.vcxproj serializer-vc10.vcxproj.filters serializer-vc11.vcxproj serializer-vc11.vcxproj.filters serializer-vc12.vcxproj serializer-vc12.vcxproj.filters

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx  
LDADD = $(top_builddir)/xml/libstudxml.la

AM_CPPFLAGS = -I'$(top_builddir)' -I'$(top_srcdir)'

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=example=; export example;

CLEANFILES = test.out

# Make sure make doesn't try to build the no-extension headers
# thinking that they are executable. Those are pulled in through
# auto-generated dependencies.
#
$(top_srcdir)/xml/content $(top_srcdir)/xml/exception $(top_srcdir)/xml/forward $(top_srcdir)/xml/qname $(top_srcdir)/xml/parser $(top_srcdir)/xml/serializer $(top_srcdir)/xml/value-traits : ; @:
