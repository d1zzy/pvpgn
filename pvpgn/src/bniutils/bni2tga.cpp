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
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include "compat/exitstatus.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <errno.h>
#include "compat/strerror.h"
#include "fileio.h"
#include "bni.h"
#include "common/version.h"
#include "common/setup_after.h"


#define BUFSIZE 1024


static void usage(char const * progname)
{
    fprintf(stderr,
            "usage: %s [<options>] [--] [<BNI file> [<TGA file>]]\n"
            "    -h, --help, --usage  show this information and exit\n"
            "    -v, --version        print version number and exit\n",progname);

    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    char const *  bnifile=NULL;
    char const *  tgafile=NULL;
    FILE *        fbni;
    FILE *        ftga;
    int           a;
    int           forcefile=0;
    char          dash[]="-"; /* unique address used as flag */
    
    if (argc<1 || !argv || !argv[0])
    {
	fprintf(stderr,"bad arguments\n");
	return STATUS_FAILURE;
    }
    
    for (a=1; a<argc; a++)
        if (forcefile && !bnifile)
            bnifile = argv[a];
        else if (strcmp(argv[a],"-")==0 && !bnifile)
            bnifile = dash;
        else if (argv[a][0]!='-' && !bnifile)
            bnifile = argv[a];
        else if (forcefile && !tgafile)
            tgafile = argv[a];
        else if (strcmp(argv[a],"-")==0 && !tgafile)
            tgafile = dash;
        else if (argv[a][0]!='-' && !tgafile)
            tgafile = argv[a];
	else if (forcefile || argv[a][0]!='-' || strcmp(argv[a],"-")==0)
	{
	    fprintf(stderr,"%s: extra file argument \"%s\"\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
	else if (strcmp(argv[a],"--")==0)
	    forcefile = 1;
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

    if (!bnifile)
	bnifile = dash;
    if (!tgafile)
	tgafile = dash;
    
    if (bnifile==dash)
	fbni = stdin;
    else
	if (!(fbni = fopen(bnifile,"r")))
	{
	    fprintf(stderr,"%s: could not open BNI file \"%s\" for reading (fopen: %s)\n",argv[0],bnifile,pstrerror(errno));
	    exit(STATUS_FAILURE);
	}
    if (tgafile==dash)
	ftga = stdout;
    else
	if (!(ftga = fopen(tgafile,"w")))
	{
	    fprintf(stderr,"%s: could not open TGA file \"%s\" for reading (fopen: %s)\n",argv[0],tgafile,pstrerror(errno));
	    exit(STATUS_FAILURE);
	}
    
    {
	unsigned char buf[BUFSIZE];
	size_t        rc;
	t_bnifile     bnih;
	
	file_rpush(fbni);
	bnih.unknown1 = file_readd_le();
	bnih.unknown2 = file_readd_le();
	bnih.numicons = file_readd_le();
	bnih.dataoffset = file_readd_le();
	fprintf(stderr,"Info: numicons=%d dataoffset=0x%08x(%d)\n",bnih.numicons,bnih.dataoffset,bnih.dataoffset);
	if (fseek(fbni,bnih.dataoffset,SEEK_SET)<0)
	{
	    fprintf(stderr,"%s: could not seek to offset %u in BNI file \"%s\" (fseek: %s)\n",argv[0],bnih.dataoffset,bnifile,pstrerror(errno));
	    return STATUS_FAILURE;
	}
	while ((rc = fread(buf,1,sizeof(buf),fbni))>0) {
	    if (fwrite(buf,rc,1,ftga) < 1) {
		fprintf(stderr,"%s: could not write data to TGA file \"%s\" (fwrite: %s)\n",argv[0],tgafile,pstrerror(errno));
		return STATUS_FAILURE;
	    }
	}
	file_rpop();
    }
    
    if (tgafile!=dash && fclose(ftga)<0)
	fprintf(stderr,"%s: could not close TGA file \"%s\" after writing (fclose: %s)\n",argv[0],tgafile,pstrerror(errno));
    if (bnifile!=dash && fclose(fbni)<0)
	fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
    
    return STATUS_SUCCESS;
}
