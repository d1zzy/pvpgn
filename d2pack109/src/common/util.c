/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/strtoul.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include "common/xalloc.h"
#include <ctype.h>
#include "common/util.h"
#include "common/setup_after.h"


extern int strstart(char const * full, char const * part)
{
    size_t strlen_full, strlen_part;
    if (!full || !part)
	return 1;
    strlen_full = strlen(full);
    strlen_part = strlen(part);
    if (strlen_full<strlen_part)
	return 1;
    /* If there is more than the command, make sure it is separated */
    if (strlen_full>strlen_part && full[strlen_part]!=' ' && full[strlen_part]!='\0')
        return 1;
    return strncasecmp(full,part,strlen_part);
}


#define DEF_LEN 64
#define INC_LEN 16

extern char * file_get_line(FILE * fp)
{
    char *       line;
    unsigned int len=DEF_LEN;
    unsigned int pos=0;
    int          prev_char,curr_char;
    
    line = xmalloc(DEF_LEN);
    prev_char = '\0';
    while ((curr_char = fgetc(fp))!=EOF)
    {
	if (((char)curr_char)=='\r')
	    continue; /* make DOS line endings look Unix-like */
	if (((char)curr_char)=='\n')
	{
	    if (pos<1 || ((char)prev_char)!='\\')
		break;
	    pos--; /* throw away the backslash */
	    prev_char = '\0';
	    continue;
	}
	prev_char = curr_char;
	
	line[pos++] = (char)curr_char;
	if ((pos+1)>=len)
	{
	    len += INC_LEN;
	    line = xrealloc(line,len);
	}
    }
    
    if (curr_char==EOF && pos<1) /* not even an empty line */
    {
	xfree(line);
	return NULL;
    }

    if (pos+1<len)
	line = xrealloc(line,pos+1); /* bump the size back down to what we need */

    line[pos] = '\0';
    
    return line;
}


extern char * strreverse(char * str)
{
    unsigned int i;
    unsigned int len;
    char         temp;
    
    if (!str)
	return NULL;
    
    len = strlen(str);
    
    /* swap leftmost and rightmost chars until we hit center */
    for (i=0; i<len/2; i++)
    {
	temp         = str[i];
	str[i]       = str[len-1-i];
	str[len-1-i] = temp;
    }
    
    return str;
}


extern int str_to_uint(char const * str, unsigned int * num)
{
    unsigned int pos;
    unsigned int i;
    unsigned int val;
    unsigned int pval;
    
    if (!str || !num)
        return -1;
    for (pos=0; str[pos]==' ' || str[pos]=='\t'; pos++);
    if (str[pos]=='+')
        pos++;
    
    val = 0;
    for (i=pos; str[i]!='\0'; i++)
    {
	pval = val;
        val *= 10;
	if (val/10!=pval) /* check for overflow */
	    return -1;
	
	pval = val;
	switch (str[i])
	{
	case '0':
	    break;
	case '1':
	    val += 1;
	    break;
	case '2':
	    val += 2;
	    break;
	case '3':
	    val += 3;
	    break;
	case '4':
	    val += 4;
	    break;
	case '5':
	    val += 5;
	    break;
	case '6':
	    val += 6;
	    break;
	case '7':
	    val += 7;
	    break;
	case '8':
	    val += 8;
	    break;
	case '9':
	    val += 9;
	    break;
	default:
	    return -1;
	}
	if (val<pval) /* check for overflow */
	    return -1;
    }
    
    *num = val;
    return 0;
}


extern int str_to_ushort(char const * str, unsigned short * num)
{
    unsigned int   pos;
    unsigned int   i;
    unsigned short val;
    unsigned short pval;
    
    if (!str || !num)
        return -1;
    for (pos=0; str[pos]==' ' || str[pos]=='\t'; pos++);
    if (str[pos]=='+')
        pos++;
    
    val = 0;
    for (i=pos; str[i]!='\0'; i++)
    {
	pval = val;
        val *= 10;
	if (val/10!=pval) /* check for overflow */
	    return -1;
	
	pval = val;
	switch (str[i])
	{
	case '0':
	    break;
	case '1':
	    val += 1;
	    break;
	case '2':
	    val += 2;
	    break;
	case '3':
	    val += 3;
	    break;
	case '4':
	    val += 4;
	    break;
	case '5':
	    val += 5;
	    break;
	case '6':
	    val += 6;
	    break;
	case '7':
	    val += 7;
	    break;
	case '8':
	    val += 8;
	    break;
	case '9':
	    val += 9;
	    break;
	default:
	    return -1;
	}
	if (val<pval) /* check for overflow */
	    return -1;
    }
    
    *num = val;
    return 0;
}


/* This routine assumes ASCII like control chars.
   If len is zero, it will print all characters up to the first NUL,
   otherwise it will print exactly that many characters. */
int str_print_term(FILE * fp, char const * str, unsigned int len, int allow_nl)
{
    unsigned int i;
    
    if (!fp)
	return -1;
    if (!str)
	return -1;
    
    if (len==0)
	len = strlen(str);
    for (i=0; i<len; i++)
	if ((str[i]=='\177' || (str[i]>='\000' && str[i]<'\040')) &&
	    (!allow_nl || (str[i]!='\r' && str[i]!='\n')))
	    fprintf(fp,"^%c",str[i]+64);
	else
	    fputc((int)str[i],fp);
    
    return 0;
}


extern int str_get_bool(char const * str)
{
    if (!str)
	return -1;
    
    if (strcasecmp(str,"true")==0 ||
	strcasecmp(str,"yes")==0 ||
	strcasecmp(str,"on")==0 ||
	strcmp(str,"1")==0)
	return 1;
    
    if (strcasecmp(str,"false")==0 ||
	strcasecmp(str,"no")==0 ||
	strcasecmp(str,"off")==0 ||
	strcmp(str,"0")==0)
	return 0;
    
    return -1;
}


extern char const * seconds_to_timestr(unsigned int totsecs)
{
    static char temp[256];
    int         days;
    int         hours;
    int         minutes;
    int         seconds;
    
    days    = totsecs/(24*60*60);
    hours   = totsecs/(60*60) - days*24;
    minutes = totsecs/60 - days*24*60 - hours*60;
    seconds = totsecs - days*24*60*60 - hours*60*60 - minutes*60;
    
    if (days>0)
	sprintf(temp,"%d day%s %d hour%s %d minute%s %d second%s",
                days,days==1 ? "" : "s",
                hours,hours==1 ? "" : "s",
                minutes,minutes==1 ? "" : "s",
                seconds,seconds==1 ? "" : "s");
    else if (hours>0)
	sprintf(temp,"%d hour%s %d minute%s %d second%s",
                hours,hours==1 ? "" : "s",
                minutes,minutes==1 ? "" : "s",
                seconds,seconds==1 ? "" : "s");
    else if (minutes>0)
	sprintf(temp,"%d minute%s %d second%s",
                minutes,minutes==1 ? "" : "s",
                seconds,seconds==1 ? "" : "s");
    else
	sprintf(temp,"%d second%s.",
                seconds,seconds==1 ? "" : "s");
    
    return temp;
}


extern int clockstr_to_seconds(char const * clockstr, unsigned int * totsecs)
{
    unsigned int i,j;
    unsigned int temp;
    
    if (!clockstr)
	return -1;
    if (!totsecs)
	return -1;
    
    for (i=j=temp=0; j<strlen(clockstr); j++)
    {
	switch (clockstr[j])
	{
	case ':':
	    temp *= 60;
	    temp += strtoul(&clockstr[i],NULL,10);
	    i = j+1;
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    break;
	default:
	    return -1;
	}
    }
    if (i<j)
    {
	temp *= 60;
	temp += strtoul(&clockstr[i],NULL,10);
    }
    
    *totsecs = temp;
    return 0;
}


extern char * escape_fs_chars(char const * in, unsigned int len)
{
    char *       out;
    unsigned int inpos;
    unsigned int outpos;
    
    if (!in)
	return NULL;
    out = xmalloc(len*3+1); /* if all turn into %XX */

    for (inpos=0,outpos=0; inpos<len; inpos++)
    {
	if (in[inpos]=='\0' || in[inpos]=='%' || in[inpos]=='/' ||
            in[inpos]=='\\' || in[inpos]==':') /* FIXME: what other characters does Windows not allow? */
	{
	    out[outpos++] = '%';
	    /* always 00 through FF hex */
	    sprintf(&out[outpos],"%02X",(unsigned int)(unsigned char)in[inpos]);
	    outpos += 2;
	}
	else
	    out[outpos++] = in[inpos];
    }
/*  if outpos >= len*3+1 then the buffer was overflowed */
    out[outpos] = '\0';
    
    return out;
}


extern char * escape_chars(char const * in, unsigned int len)
{
    char *       out;
    unsigned int inpos;
    unsigned int outpos;
    
    if (!in)
	return NULL;
    out = xmalloc(len*4+1); /* if all turn into \xxx */

    for (inpos=0,outpos=0; inpos<len; inpos++)
    {
	if (in[inpos]=='\\')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = '\\';
	}
	else if (in[inpos]=='"')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = '"';
	}
        else if (isascii((int)in[inpos]) && isprint((int)in[inpos]))
	    out[outpos++] = in[inpos];
        else if (in[inpos]=='\a')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'a';
	}
        else if (in[inpos]=='\b')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'b';
	}
        else if (in[inpos]=='\t')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 't';
	}
        else if (in[inpos]=='\n')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'n';
	}
        else if (in[inpos]=='\v')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'v';
	}
        else if (in[inpos]=='\f')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'f';
	}
        else if (in[inpos]=='\r')
	{
	    out[outpos++] = '\\';
	    out[outpos++] = 'r';
	}
	else
	{
	    out[outpos++] = '\\';
	    /* always 001 through 377 octal */
	    sprintf(&out[outpos],"%03o",(unsigned int)(unsigned char)in[inpos]);
	    outpos += 3;
	}
    }
/*  if outpos >= len*4+1 then the buffer was overflowed */
    out[outpos] = '\0';
    
    return out;
}


extern char * unescape_chars(char const * in)
{
    char *       out;
    unsigned int inpos;
    unsigned int outpos;
    
    if (!in)
	return NULL;
    out = xmalloc(strlen(in)+1);

    for (inpos=0,outpos=0; inpos<strlen(in); inpos++)
    {
        if (in[inpos]!='\\')
	    out[outpos++] = in[inpos];
        else
	    switch (in[++inpos])
	    {
	    case '\\':
		out[outpos++] = '\\';
		break;
	    case '"':
		out[outpos++] = '"';
		break;
	    case 'a':
		out[outpos++] = '\a';
		break;
	    case 'b':
		out[outpos++] = '\b';
		break;
	    case 't':
		out[outpos++] = '\t';
		break;
	    case 'n':
		out[outpos++] = '\n';
		break;
	    case 'v':
		out[outpos++] = '\v';
		break;
	    case 'f':
		out[outpos++] = '\f';
		break;
	    case 'r':
		out[outpos++] = '\r';
		break;
	    default:
	    {
		char         temp[4];
		unsigned int i;
		unsigned int num;
		
		for (i=0; i<3; i++)
		{
		    if (in[inpos]!='0' &&
		        in[inpos]!='1' &&
		        in[inpos]!='2' &&
		        in[inpos]!='3' &&
		        in[inpos]!='4' &&
		        in[inpos]!='5' &&
		        in[inpos]!='6' &&
		        in[inpos]!='7')
			break;
		    temp[i] = in[inpos++];
		}
		temp[i] = '\0';
		inpos--;
		
		num = strtoul(temp,NULL,8);
		if (i<3 || num<1 || num>255) /* bad escape (including \000), leave it as-is */
		{
		    out[outpos++] = '\\';
		    strcpy(&out[outpos],temp);
		    outpos += strlen(temp);
		}
		else
		    out[outpos++] = (unsigned char)num;
	    }
	}
    }
    out[outpos] = '\0';
    
    return out;
}


extern void str_to_hex(char * target, char * data, int datalen)
{
    unsigned char c;
    int  i;
    for (i = 0; i < datalen; i++)
    {
        c = (data[i]) & 0xff;

	sprintf(target + i*3, "%02X    ", c);
	target[i*3+3] = '\0';

	/* fprintf(stderr, "str_to_hex %d | '%02x' '%s'\n", i, c, target); */
    }
}


extern int hex_to_str(char const * source, char * data, int datalen)
{
   /*
    * TODO: We really need a more robust function here,
    *       for now, I'll just use this hack for a quick evaluation
    */
    char byte;
    char c;
    int  i;
  
    for (i = 0; i < datalen; i++)
    {
	byte  = 0;
    
	/* fprintf(stderr, "hex_to_str %d | '%02x'", i, byte); */

	c = source [i*3 + 0];
	byte += 16 * ( c > '9' ? (c - 'A' + 10) : (c - '0'));

	/* fprintf(stderr, " | '%c' '%02x'", c, byte); */

	c = source [i*3 + 1];
	byte +=  1 * ( c > '9' ? (c - 'A' + 10) : (c - '0'));

	/* fprintf(stderr, " | '%c' '%02x'", c, byte); */

	/* fprintf(stderr, "\n"); */

	data[i] = byte;
    }
    
    /* fprintf(stderr, "finished, returning %d from '%s'\n", i, source);  */
    
    return i;
}
