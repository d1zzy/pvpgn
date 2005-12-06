AM_CPPFLAGS=-I$(top_srcdir)/src

bin_PROGRAMS = bnilist bni2tga bniextract bnibuild tgainfo

bnilist_SOURCES = bnilist.c fileio.c tga.c
bnilist_LDADD = $(top_builddir)/src/compat/libcompat.a

bni2tga_SOURCES = bni2tga.c fileio.c

bniextract_SOURCES = bniextract.c fileio.c tga.c bni.c
bniextract_LDADD = $(top_builddir)/src/compat/libcompat.a

bnibuild_SOURCES = bnibuild.c fileio.c bni.c tga.c
bnibuild_LDADD = $(top_builddir)/src/compat/libcompat.a

tgainfo_SOURCES = tgainfo.c fileio.c tga.c
tgainfo_LDADD = $(top_builddir)/src/compat/libcompat.a

noinst_HEADERS = bni.h fileio.h tga.h
