/*
    Copyright (C) 2000  Marco Ziech (mmz@gmx.net)
    Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "common/setup_before.h"
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/uint.h"
#include "fileio.h"
#include "common/setup_after.h"

typedef struct t_file {
	FILE *f;
	struct t_file *next;
} t_file;

static t_file *r_file = NULL;
static t_file *w_file = NULL;

/* ----------------------------------------------------------------- */

extern void file_rpush(FILE *f) {
	t_file *tf = (t_file*)malloc(sizeof(t_file));
	tf->next = r_file;
	tf->f = f;
	r_file = tf;
	return;
}

extern void file_rpop(void) {
	t_file *tf;
	tf = r_file;
	r_file = tf->next;
	free(tf);
	return;
}

/* ----------------------------------------------------------------- */

extern void file_wpush(FILE *f) {
	t_file *tf = (t_file*)malloc(sizeof(t_file));
	tf->next = w_file;
	tf->f = f;
	w_file = tf;
	return;
}

extern void file_wpop(void) {
	t_file *tf;
	tf = w_file;
	w_file = tf->next;
	free(tf);
	return;
}

/* ----------------------------------------------------------------- */

extern t_uint8 file_readb(void) {
	unsigned char buff[1];
	if (fread(buff,1,sizeof(buff),r_file->f) < sizeof(buff)) {
		if (ferror(r_file->f)) perror("file_readb: fread");
		return 0;
	}
	return (((t_uint8)buff[0])    );
}


extern t_uint16 file_readw_le(void) {
	unsigned char buff[2];
	if (fread(buff,1,sizeof(buff),r_file->f) < sizeof(buff)) {
		if (ferror(r_file->f)) perror("file_readw_le: fread");
		return 0;
	}
	return (((t_uint16)buff[0])    )|
               (((t_uint16)buff[1])<< 8);
}


extern t_uint16 file_readw_be(void) {
	unsigned char buff[2];
	if (fread(buff,1,sizeof(buff),r_file->f) < sizeof(buff)) {
		if (ferror(r_file->f)) perror("file_readw_be: fread");
		return 0;
	}
	return (((t_uint16)buff[0])<< 8)|
               (((t_uint16)buff[1])    );
}


extern t_uint32 file_readd_le(void) {
	unsigned char buff[4];
	if (fread(buff,1,sizeof(buff),r_file->f) < sizeof(buff)) {
		if (ferror(r_file->f)) perror("file_readd_le: fread");
		return 0;
	}
	return (((t_uint32)buff[0])    )|
               (((t_uint32)buff[1])<< 8)|
               (((t_uint32)buff[2])<<16)|
               (((t_uint32)buff[3])<<24);
}


extern t_uint32 file_readd_be(void) {
	unsigned char buff[4];
	if (fread(buff,1,sizeof(buff),r_file->f) < sizeof(buff)) {
		if (ferror(r_file->f)) perror("file_readd_be: fread");
		return 0;
	}
	return (((t_uint32)buff[0])<<24)|
               (((t_uint32)buff[1])<<16)|
               (((t_uint32)buff[2])<< 8)|
               (((t_uint32)buff[3])    );
}


extern int file_writeb(t_uint8 u) {
	unsigned char buff[1];
	buff[0] = (u   );
	if (fwrite(buff,1,sizeof(buff),w_file->f) < sizeof(buff)) {
		if (ferror(w_file->f)) perror("file_writeb: fwrite");
		return -1;
	}
	return 0;
}


extern int file_writew_le(t_uint16 u) {
	unsigned char buff[2];
	buff[0] = (u    );
	buff[1] = (u>> 8);
	if (fwrite(buff,1,sizeof(buff),w_file->f) < sizeof(buff)) {
		if (ferror(w_file->f)) perror("file_writew_le: fwrite");
		return -1;
	}
	return 0;
}


extern int file_writew_be(t_uint16 u) {
	unsigned char buff[2];
	buff[0] = (u>> 8);
	buff[1] = (u    );
	if (fwrite(buff,1,sizeof(buff),w_file->f) < sizeof(buff)) {
		if (ferror(w_file->f)) perror("file_writew_be: fwrite");
		return -1;
	}
	return 0;
}


extern int file_writed_le(t_uint32 u) {
	unsigned char buff[4];
	buff[0] = (u    );
	buff[1] = (u>> 8);
	buff[2] = (u>>16);
	buff[3] = (u>>24);
	if (fwrite(buff,1,sizeof(buff),w_file->f) < sizeof(buff)) {
		if (ferror(w_file->f)) perror("file_writed_le: fwrite");
		return -1;
	}
	return 0;
}


extern int file_writed_be(t_uint32 u) {
	unsigned char buff[4];
	buff[0] = (u>>24);
	buff[1] = (u>>16);
	buff[2] = (u>> 8);
	buff[3] = (u    );
	if (fwrite(buff,1,sizeof(buff),w_file->f) < sizeof(buff)) {
		if (ferror(w_file->f)) perror("file_writed_be: fwrite");
		return -1;
	}
	return 0;
}
