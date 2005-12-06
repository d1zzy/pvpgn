AM_CPPFLAGS=-I$(top_srcdir)/src -DD2CS_DEFAULT_CONF_FILE=\"$(sysconfdir)/d2cs.conf\"

sbin_PROGRAMS = d2cs

d2cs_SOURCES = bnetd.c cmdline.c connection.c d2charfile.c d2charlist.c \
	d2gs.c d2ladder.c game.c gamequeue.c handle_bnetd.c handle_d2cs.c \
	handle_d2gs.c handle_init.c handle_signal.c main.c net.c prefs.c \
	s2s.c server.c serverqueue.c
d2cs_LDADD = $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

noinst_HEADERS = bit.h bnetd.h cmdline.h connection.h d2charfile.h \
	d2charlist.h d2cs_bnetd_protocol.h d2cs_d2dbs_ladder.h \
	d2cs_d2gs_character.h d2cs_d2gs_protocol.h d2cs_protocol.h d2gs.h \
	d2ladder.h game.h gamequeue.h handle_bnetd.h handle_d2cs.h \
	handle_d2gs.h handle_init.h handle_signal.h net.h prefs.h s2s.h \
	server.h serverqueue.h setup.h version.h
