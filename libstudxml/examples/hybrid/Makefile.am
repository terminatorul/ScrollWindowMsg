# file      : examples/template/Makefile.am
# copyright : not copyrighted - public domain

EXTRA_DIST = README position.xml hybrid-vc9.vcproj hybrid-vc10.vcxproj hybrid-vc10.vcxproj.filters hybrid-vc11.vcxproj hybrid-vc11.vcxproj.filters hybrid-vc12.vcxproj hybrid-vc12.vcxproj.filters

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx dom.cxx dom.hxx
LDADD = $(top_builddir)/xml/libstudxml.la

AM_CPPFLAGS = -I'$(top_builddir)' -I'$(top_srcdir)'

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=example=1; export example;

# Make sure make doesn't try to build the no-extension headers
# thinking that they are executable. Those are pulled in through
# auto-generated dependencies.
#
$(top_srcdir)/xml/content $(top_srcdir)/xml/exception $(top_srcdir)/xml/forward $(top_srcdir)/xml/qname $(top_srcdir)/xml/parser $(top_srcdir)/xml/serializer $(top_srcdir)/xml/value-traits : ; @:
