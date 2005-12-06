/*
 * Copyright (C) 1999  Descolada (dyn1-tnt9-237.chicago.il.ameritech.net)
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "common/setup_before.h"
#include <stdio.h>
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/uint.h"
#include "common/introtate.h"
#include "common/eventlog.h"
#include "common/bnethash.h"
#include "common/setup_after.h"


static void hash_init(t_hash * hash);
static void do_hash(t_hash * hash, t_uint32 * tmp);
static void hash_set_16(t_uint32 * dst, unsigned char const * src, unsigned int count);


static void hash_init(t_hash * hash)
{
    (*hash)[0] = 0x67452301;
    (*hash)[1] = 0xefcdab89;
    (*hash)[2] = 0x98badcfe;
    (*hash)[3] = 0x10325476;
    (*hash)[4] = 0xc3d2e1f0;
}


static void do_hash(t_hash * hash, t_uint32 * tmp)
{
    unsigned int i;
    t_uint32     a,b,c,d,e,g;
    
    for (i=0; i<64; i++)
	tmp[i+16] = ROTL32(1,tmp[i] ^ tmp[i+8] ^ tmp[i+2] ^ tmp[i+13]);
    
    a = (*hash)[0];
    b = (*hash)[1];
    c = (*hash)[2];
    d = (*hash)[3];
    e = (*hash)[4];
    
    for (i=0; i<20*1; i++)
    {
	g = tmp[i] + ROTL32(a,5) + e + ((b & c) | (~b & d)) + 0x5a827999;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }
    
    for (; i<20*2; i++)
    {
	g = (d ^ c ^ b) + e + ROTL32(g,5) + tmp[i] + 0x6ed9eba1;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }
    
    for (; i<20*3; i++)
    {
	g = tmp[i] + ROTL32(g,5) + e + ((c & b) | (d & c) | (d & b)) - 0x70e44324;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }
    
    for (; i<20*4; i++)
    {
	g = (d ^ c ^ b) + e + ROTL32(g,5) + tmp[i] - 0x359d3e2a;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }
    
    (*hash)[0] += g;
    (*hash)[1] += b;
    (*hash)[2] += c;
    (*hash)[3] += d;
    (*hash)[4] += e;
}


/*
 * Fill 16 elements of the array of 32 bit values with the bytes from
 * dst up to count in little endian order. Fill left over space with
 * zeros
 */
static void hash_set_16(t_uint32 * dst, unsigned char const * src, unsigned int count)
{
    unsigned int i;
    unsigned int pos;
    
    for (pos=0,i=0; i<16; i++)
    {
	dst[i] = 0;
        if (pos<count)
	    dst[i] |= ((t_uint32)src[pos]);
	pos++;
        if (pos<count)
	    dst[i] |= ((t_uint32)src[pos])<<8;
	pos++;
        if (pos<count)
	    dst[i] |= ((t_uint32)src[pos])<<16;
	pos++;
        if (pos<count)
	    dst[i] |= ((t_uint32)src[pos])<<24;
	pos++;
    }
}


extern int bnet_hash(t_hash * hashout, unsigned int size, void const * datain)
{
    t_uint32              tmp[64+16];
    unsigned char const * data;
    unsigned int          inc;
    
    if (!hashout || !*hashout)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL hashout");
	return -1;
    }
    if (size>0 && !datain)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL datain with size=%u",size);
	return -1;
    }
    
    hash_init(hashout);
    
    data = (const unsigned char*)datain;
    while (size>0)
    {
	if (size>64)
	    inc = 64;
	else
	    inc = size;
	
	hash_set_16(tmp,data,inc);
	do_hash(hashout,tmp);
	
	data += inc;
	size -= inc;
    }
    
    return 0;
}


extern int hash_eq(t_hash const h1, t_hash const h2)
{
    unsigned int i;
    
    if (!h1 || !h2)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL hash");
	return -1;
    }
    
    for (i=0; i<5; i++)
	if (h1[i]!=h2[i])
	    return 0;
    
    return 1;
}


extern char const * hash_get_str(t_hash const hash)
{
    static char  temp[8*5+1]; /* each of 5 ints to 8 chars + null */
    unsigned int i;
    
    if (!hash)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL hash");
	return NULL;
    }
    
    for (i=0; i<5; i++)
        sprintf(&temp[i*8],"%08x",hash[i]);
    
    return temp;
}


extern int hash_set_str(t_hash * hash, char const * str)
{
    unsigned int i;
    
    if (!hash)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL hash pointer");
        return -1;
    }
    if (!*hash)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL hash");
        return -1;
    }
    if (!str)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL str");
        return -1;
    }
    if (strlen(str)!=5*8)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got string with length %u (should be %u)",strlen(str),5*8);
        return -1;
    }
    
    for (i=0; i<5; i++)
        if (sscanf(&str[i*8],"%8x",&(*hash)[i])!=1)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"got bad string");
	    return -1;
	}
    
    return 0;
}


