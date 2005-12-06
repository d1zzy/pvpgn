AM_CPPFLAGS=-I$(top_srcdir)/src

noinst_LIBRARIES = libtinycdb.a
libtinycdb_a_SOURCES = cdb_find.c cdb_findnext.c cdb_hash.c cdb_init.c \
	cdb_make_add.c cdb_make.c cdb_make_find.c cdb_make_put.c cdb_seek.c \
	cdb_seq.c cdb_unpack.c

bin_PROGRAMS = bncdb
bncdb_SOURCES = cdb.c
bncdb_LDADD = libtinycdb.a $(top_builddir)/src/common/libcommon.a \
	$(top_builddir)/src/compat/libcompat.a

noinst_HEADERS = cdb.h cdb_int.h
