/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#include <stdio.h>
#include "compat/exitstatus.h"
#include <ctype.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "common/eventlog.h"
#include "common/bnethash.h"
#include "common/version.h"
#include "common/setup_after.h"


static void usage(char const * progname)
{
    fprintf(stderr,
            "usage: %s [<options>] [--] [<cleartextpassword>]\n"
            "    -h, --help, --usage  show this information and exit\n"
            "    -v, --version        print version number and exit\n",progname);
    
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    
    char const * pass=NULL;
    int          a;
    int          forcepass=0;
    
    if (argc<1 || !argv || !argv[0])
    {
        fprintf(stderr,"bad arguments\n");
        return STATUS_FAILURE;
    }
    
    for (a=1; a<argc; a++)
        if (forcepass && !pass)
            pass = argv[a];
        else if (strcmp(argv[a],"-")==0 && !pass)
            pass = argv[a];
        else if (argv[a][0]!='-' && !pass)
            pass = argv[a];
        else if (forcepass || argv[a][0]!='-' || strcmp(argv[a],"-")==0)
        {
            fprintf(stderr,"%s: extra password argument \"%s\"\n",argv[0],argv[a]);
            usage(argv[0]);
        }
        else if (strcmp(argv[a],"--")==0)
            forcepass = 1;
        else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
        {
            printf("version "PVPGN_VERSION"\n");
            return STATUS_SUCCESS;
        }
        else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")
==0)
            usage(argv[0]);
        else
        {
            fprintf(stderr,"%s: unknown option \"%s\"\n",argv[0],argv[a]);
            usage(argv[0]);
        }
    
    {
	char         buff[256];
	t_hash       hash;
	unsigned int i;
	
	eventlog_set(stderr); /* bnet_hash() and friends use eventlog */
	
	if (!pass)
	{
	    printf("Enter password to hash: ");
	    fflush(stdout);
	    fgets(buff,256,stdin);
	    if (buff[0]!='\0')
	        buff[strlen(buff)-1] = '\0';
	}
	else
	{
	    strncpy(buff,pass,sizeof(buff));
	    buff[sizeof(buff)-1] = '\0';
	}
	
	/* FIXME: what is the max password length? */
	for (i=0; i<strlen(buff); i++)
	    if (isascii((int)buff[i]) && isupper((int)buff[i])) /* some tolower()'s are broken */
		buff[i] = tolower((int)buff[i]);
	
	bnet_hash(&hash,strlen(buff),buff);
	printf("\"BNET\\\\acct\\\\passhash1\"=\"%s\"\n",hash_get_str(hash));
    }
    
    return STATUS_SUCCESS;
}
