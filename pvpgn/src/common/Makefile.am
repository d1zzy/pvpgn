AM_CPPFLAGS=-I$(top_srcdir)/src

noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = conf.c list.c eventlog.c hexdump.c bn_type.c util.c \
	addr.c d2char_checksum.c xalloc.c network.c packet.c xstring.c \
	asnprintf.c bnethash.c bnethashconv.c bnettime.c bn_type.c \
	fdwatch.c fdwatch_epoll.c fdwatch_kqueue.c fdwatch_poll.c \
	fdwatch_select.c give_up_root_privileges.c hashtable.c \
	proginfo.c queue.c rcm.c rlimit.c tag.c token.c trans.c

noinst_HEADERS = addr.h anongame_protocol.h asnprintf.h bnethashconv.h \
	bnethash.h bnet_protocol.h bnettime.h bn_type.h bot_protocol.h \
	conf.h d2char_checksum.h d2char_file.h d2game_protocol.h elist.h \
	eventlog.h fdwatch_epoll.h fdwatch.h fdwatch_kqueue.h fdwatch_poll.h \
	fdwatch_select.h field_sizes.h file_protocol.h flags.h hashtable.h \
	give_up_root_privileges.h hexdump.h init_protocol.h introtate.h \
	irc_protocol.h list.h lstr.h network.h packet.h proginfo.h queue.h \
	rcm.h rlimit.h setup_after.h setup_before.h tag.h token.h tracker.h \
	trans.h udp_protocol.h util.h version.h xalloc.h xstring.h
