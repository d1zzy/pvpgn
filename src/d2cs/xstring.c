/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
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
#include "setup.h"

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/memcpy.h"
#include "compat/memmove.h"
#include "compat/strdup.h"

#include "common/xalloc.h"
#include "xstring.h"
#include "common/setup_after.h"

extern char * strtolower(char * str)
{
	unsigned int	i;
	unsigned char	ch;

	if (!str) return NULL;
	for (i=0; (ch=str[i]); i++) {
		if ((isupper(ch))) str[i]=ch+('a'-'A');
	}
	return str;
}

extern unsigned char xtoi(unsigned char ch)
{
	unsigned char retval;

	if (isalpha(ch)) retval=tolower(ch);
	else retval=ch;
	if (retval < 'A') retval -= ('0'-0);
	else retval -= ('a' - 0xa);
	return retval;
}

extern char * str_strip_affix(char * str, char const * affix)
{
	unsigned int i, j, n;
	int		match;

	if (!str) return NULL;
	if (!affix) return str;
	for (i=0; str[i]; i++) {
		match=0;
		for (n=0; affix[n]; n++) {
			if (str[i]==affix[n]) {
				match=1;
				break;
			}
		}
		if (!match) break;
	}
	for (j=strlen(str)-1; j>=i; j--) {
		match=0;
		for (n=0; affix[n]; n++) {
			if (str[j]==affix[n]) {
				match=1;
				break;
			}
		}
		if (!match) break;
	}
	if (i>j) {
		str[0]='\0';
	} else {
		memmove(str,str+i,j-i+1);
		str[j-i+1]='\0';
	}
	return str;
}

extern char * hexstrdup(unsigned char const * src)
{
	char	* dest;
	int	len;

	if (!src) return NULL;
	dest=xstrdup(src);
	len=hexstrtoraw(src,dest,strlen(dest)+1);
	dest[len]='\0';
	return dest;
}

extern unsigned int hexstrtoraw(unsigned char const * src, char * data, unsigned int datalen)
{
	unsigned char	ch;
	unsigned int	i, j;

	for (i=0, j=0; j<datalen; i++) {
		ch=src[i];
		if (!ch) break;
		if (ch=='\\') {
			i++;
			ch=src[i];
			if (!ch) {
				break;
			} else if (ch=='\\') {
				data[j++]=ch;
			} else if (ch=='x') {
				if (isxdigit(src[i+1])) { 
					if (isxdigit(src[i+2])) {
						data[j++]=xtoi(src[i+1]) * 0x10 + xtoi(src[i+2]);
						i+=2;
					} else {
						data[j++]=xtoi(src[i+1]);
						i++;
					}
				} else {
					data[j++]=ch;
				}
			} else if (ch=='n') {
				data[j++]='\n';
			} else if (ch=='r') {
				data[j++]='\r';
			} else if (ch=='a') {
				data[j++]='\a';
			} else if (ch=='t') {
				data[j++]='\t';
			} else if (ch=='b') {
				data[j++]='\b';
			} else if (ch=='f') {
				data[j++]='\f';
			} else if (ch=='v') {
				data[j++]='\v';
			} else {
				data[j++]=ch;
			}
		} else {
			data[j++]=ch;
			continue;
		}
	}
	return j;
}

#define SPLIT_STRING_INIT_COUNT		32
#define	SPLIT_STRING_INCREASEMENT	32
extern char * * strtoargv(char const * str, unsigned int * count)
{
	unsigned int	n, index_size;
	char		* temp;
	unsigned int	i;
	int		j;
	int		* pindex;
	void		** ptrindex;
	char		* result;

	if (!str || !count) return NULL;
	temp=xmalloc(strlen(str)+1);
	n = SPLIT_STRING_INIT_COUNT;
	pindex=xmalloc(n * sizeof (int));

	i=j=0;
	*count=0;
	while (str[i]) {
		while (str[i]==' ' || str[i]=='\t') i++;
		if (!str[i]) break;
		if (*count >=n ) {
			n += SPLIT_STRING_INCREASEMENT;
			pindex=(int *)xrealloc(pindex,n * sizeof(int));
		}
		pindex[*count]=j;
		(*count)++;
		if (str[i]=='"') {
			i++;
			while (str[i]) {
				if (str[i]=='\\') {
					i++;
					if (!str[i]) break;
				} else if (str[i]=='"') {
					i++;
					break;
				}
				temp[j++]=str[i++];
			}
		} else {
			while (str[i] && str[i] != ' ' && str[i] != '\t') {
				temp[j++]=str[i++];
			}
		}
		temp[j++]='\0';
	}
	index_size= *count * sizeof(char *);
	if (!index_size) {
		xfree(temp);
		xfree(pindex);
		return NULL;
	}
	result=xmalloc(j+index_size);
	memcpy(result+index_size,temp,j);

	ptrindex=xmalloc(*count * sizeof (char*));
	for (i=0; i< *count; i++) {
		ptrindex[i] = result + index_size + pindex[i];
	}
	memcpy(result,ptrindex,index_size);
	xfree(temp);
	xfree(pindex);
	xfree(ptrindex);
	return (char * *)result;
}

#define COMBINE_STRING_INIT_LEN		1024
#define COMBINE_STRING_INCREASEMENT	1024
extern char * arraytostr(char * * array, char const * delim, int count)
{
	int	i;
	unsigned int n;
	char	* result;
	int	need_delim;

	if (!delim || !array) return NULL;

	n=COMBINE_STRING_INIT_LEN;
	result=xmalloc(n);
	result[0]='\0';

	need_delim=0;
	for (i=0; i<count; i++) {
		if (!array[i]) continue;	
		if (strlen(result)+strlen(array[i])+strlen(delim)>=n) {
			n+=COMBINE_STRING_INCREASEMENT;
			result=xrealloc(result,n);
		}
		if (need_delim) {
			strcat(result,delim);
		}
		strcat(result,array[i]);
		need_delim=1;
	}
	result=xrealloc(result,strlen(result)+1);
	return result;
}

