# file      : examples/template/Makefile.am
# copyright : not copyrighted - public domain

EXTRA_DIST = README position.xml roundtrip-vc9.vcproj roundtrip-vc10.vcxproj roundtrip-vc10.vcxproj.filters roundtrip-vc11.vcxproj roundtrip-vc11.vcxproj.filters roundtrip-vc12.vcxproj roundtrip-vc12.vcxproj.filters

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx  
LDADD = $(top_builddir)/xml/libstudxml.la

AM_CPPFLAGS = -I'$(top_builddir)' -I'$(top_srcdir)'

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=example=1; export example;

# Make sure make doesn't try to build the no-extension headers
# thinking that they are executable. Those are pulled in through
# auto-generated dependencies.
#
$(top_srcdir)/xml/content $(top_srcdir)/xml/exception $(top_srcdir)/xml/forward $(top_srcdir)/xml/qname $(top_srcdir)/xml/parser $(top_srcdir)/xml/serializer $(top_srcdir)/xml/value-traits : ; @:
