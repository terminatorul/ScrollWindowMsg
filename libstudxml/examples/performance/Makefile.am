# file      : examples/template/Makefile.am
# copyright : not copyrighted - public domain

EXTRA_DIST = README gen.cxx expat.cxx test.xsd test-50k.xml performance-vc9.vcproj performance-vc10.vcxproj performance-vc10.vcxproj.filters performance-vc11.vcxproj performance-vc11.vcxproj.filters performance-vc12.vcxproj performance-vc12.vcxproj.filters

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx time.cxx time.hxx
LDADD = $(top_builddir)/xml/libstudxml.la

AM_CPPFLAGS = -I'$(top_builddir)' -I'$(top_srcdir)'

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=example=1; export example;

# Make sure make doesn't try to build the no-extension headers
# thinking that they are executable. Those are pulled in through
# auto-generated dependencies.
#
$(top_srcdir)/xml/content $(top_srcdir)/xml/exception $(top_srcdir)/xml/forward $(top_srcdir)/xml/qname $(top_srcdir)/xml/parser $(top_srcdir)/xml/serializer $(top_srcdir)/xml/value-traits : ; @:
