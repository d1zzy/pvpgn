AM_CPPFLAGS=-I$(top_srcdir)/src

bin_PROGRAMS = bnchat bnftp bnbot bnstat

bnchat_SOURCES = bnchat.c client.c client_connect.c udptest.c
bnchat_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

bnftp_SOURCES = bnftp.c client.c
bnftp_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

bnbot_SOURCES = bnbot.c client.c
bnbot_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

bnstat_SOURCES = bnstat.c client.c client_connect.c udptest.c
bnstat_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

noinst_HEADERS = ansi_term.h client_connect.h client.h udptest.h
