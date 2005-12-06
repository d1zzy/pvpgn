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
#include "tga.h"
#include "fileio.h"
#include "common/version.h"
#include "common/setup_after.h"


static void usage(char const * progname)
{
    fprintf(stderr,
            "usage: %s [<options>] [--] [<BNI file>]\n"
            "    -h, --help, --usage  show this information and exit\n"
            "    -v, --version        print version number and exit\n",progname);
    
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    char const * bnifile=NULL;
    FILE *       fp;
    int          a;
    int          forcefile=0;
    char         dash[]="-"; /* unique address used as flag */
    
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
    
    if (bnifile==dash)
	fp = stdin;
    else
	if (!(fp = fopen(bnifile,"r")))
	{
	    fprintf(stderr,"%s: could not open BNI file \"%s\" for reading (fopen: %s)\n",argv[0],bnifile,pstrerror(errno));
	    exit(STATUS_FAILURE);
	}
    
    {
	t_tgaimg * tgaimg;
	int        i;
	int        bniid, unknown, icons, datastart;
	int        expected_width, expected_height;
	
	file_rpush(fp);
	bniid = file_readd_le();
	unknown = file_readd_le();
	icons = file_readd_le();
	datastart = file_readd_le();
	fprintf(stderr,"BNIHeader: id=0x%08x unknown=0x%08x icons=0x%08x datastart=0x%08x\n",bniid,unknown,icons,datastart);
	expected_width = 0;
	expected_height = 0;
	for (i = 0; i < icons; i++) {
		int id,x,y,flags,tag;
		id = file_readd_le();
		x = file_readd_le();
		y = file_readd_le();
		if (id == 0) {
			tag = file_readd_le();
		} else {
			tag = 0;
		}
		flags = file_readd_le();
		fprintf(stderr,"Icon[%d]: id=0x%08x x=%d y=%d tag=0x%08x(\"%c%c%c%c\") flags=0x%08x\n",i,id,x,y,tag,
			((unsigned char)((tag >> 24) & 0xff)),
			((unsigned char)((tag >> 16) & 0xff)),
			((unsigned char)((tag >> 8) & 0xff)),
			((unsigned char)((tag) & 0xff)),flags);
		if (x > expected_width) expected_width = x;
		expected_height += y;
	}
	if (ftell(fp)!=datastart) {
		fprintf(stderr,"Warning: garbage after header (pos=0x%lx-datastart=0x%lx) = %ld bytes of garbage! \n",(unsigned long)ftell(fp),(unsigned long)datastart,(long)(ftell(fp)-datastart));
	}
	tgaimg = load_tgaheader();
	print_tga_info(tgaimg,stdout);
	fprintf(stderr,"\n");
	fprintf(stderr,"Check: Expected %dx%d TGA, got %ux%u. %s\n",expected_width,expected_height,tgaimg->width,tgaimg->height,((tgaimg->width == expected_width)&&(tgaimg->height == expected_height)) ? "OK." : "FAIL.");
	fprintf(stderr,"Check: Expected 24bit color depth TGA, got %dbit. %s\n",tgaimg->bpp,(tgaimg->bpp == 24) ? "OK." : "FAIL.");
	fprintf(stderr,"Check: Expected ImageType 10, got %d. %s\n",tgaimg->imgtype,(tgaimg->imgtype == 10) ? "OK." : "FAIL.");
	file_rpop();
    }
    
    if (bnifile!=dash && fclose(fp)<0)
	fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
    return STATUS_SUCCESS;
}
