AM_CPPFLAGS=-I$(top_srcdir)/src

sbin_PROGRAMS = bntrackd

bntrackd_SOURCES = bntrackd.c

bntrackd_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a
