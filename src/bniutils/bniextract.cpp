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
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/statmacros.h"
#include "compat/mkdir.h"
#include "common/version.h"
#include "fileio.h"
#include "tga.h"
#include "bni.h"
#include "common/setup_after.h"


/* extract a portion of an image creating a new image */
static t_tgaimg * area2img(t_tgaimg *src, int x, int y, int width, int height, t_tgaimgtype type) {
	t_tgaimg *dst;
	int pixelsize;
	unsigned char *datap;
	unsigned char *destp;
	int i;

	if (src == NULL) return NULL;
	if ((x+width)>src->width) return NULL;
	if ((y+height)>src->height) return NULL;
	pixelsize = getpixelsize(src);
	if (pixelsize == 0) return NULL;
	
	dst = new_tgaimg(width,height,src->bpp,type);
	dst->data = (t_uint8*)malloc(width*height*pixelsize);
	
	datap = src->data;
	datap += y*src->width*pixelsize;
	destp = dst->data;
	for (i = 0; i < height; i++) {
		datap += x*pixelsize;
		memcpy(destp,datap,width*pixelsize);
		destp += width*pixelsize;
		datap += (src->width-x)*pixelsize;
	}
	return dst;
}


static void usage(char const * progname)
{
    fprintf(stderr,
	    "usage: %s [<options>] [--] <BNI file> <output directory>\n"
            "    -h, --help, --usage  show this information and exit\n"
            "    -v, --version        print version number and exit\n",progname);

    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    char const * outdir=NULL;
    char const * bnifile=NULL;
    FILE *       fbni;
    struct stat  s;
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
        else if (forcefile && !outdir)
            outdir = argv[a];
        else if (strcmp(argv[a],"-")==0 && !outdir)
            outdir = dash;
        else if (argv[a][0]!='-' && !outdir)
            outdir = argv[a];
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
    {
	fprintf(stderr,"%s: BNI file not specified\n",argv[0]);
	usage(argv[0]);
    }
    if (!outdir)
    {
	fprintf(stderr,"%s: output directory not specified\n",argv[0]);
	usage(argv[0]);
    }
    
    if (bnifile==dash)
        fbni = stdin;
    else
	if (!(fbni = fopen(bnifile,"r")))
	{
	    fprintf(stderr,"%s: could not open BNI file \"%s\" for reading (fopen: %s)\n",argv[0],bnifile,pstrerror(errno));
	    return STATUS_FAILURE;
	}
    
    if (outdir==dash)
    {
	fprintf(stderr,"%s: can not write directory to <stdout>\n",argv[0]);
	if (bnifile!=dash && fclose(fbni)<0)
	    fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
	return STATUS_FAILURE;
    }
    if (stat(outdir,&s)<0) {
	if (errno == ENOENT) {
	    fprintf(stderr,"Info: Creating directory \"%s\" ...\n",outdir);
	    if (p_mkdir(outdir,S_IRWXU+S_IRWXG+S_IRWXO)<0) {
		fprintf(stderr,"%s: could not create output directory \"%s\" (mkdir: %s)",argv[0],outdir,pstrerror(errno));
		if (bnifile!=dash && fclose(fbni)<0)
		    fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
		return STATUS_FAILURE;
	    }
	} else {
	    fprintf(stderr,"%s: could not stat output directory \"%s\" (stat: %s)\n",argv[0],outdir,pstrerror(errno));
	    if (bnifile!=dash && fclose(fbni)<0)
		fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
	    return STATUS_FAILURE;
	}
    }
    else
	if (S_ISDIR(s.st_mode) == 0) {
	    fprintf(stderr,"%s: \"%s\" is not a directory\n",argv[0],outdir);
	    if (bnifile!=dash && fclose(fbni)<0)
		fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
	    return STATUS_FAILURE;
	}
    
    {
	unsigned int i;
	int          curry;
	t_tgaimg *   iconimg;
	t_bnifile *  bni;
	FILE *       indexfile;
	char *       indexfilename;
	
	fprintf(stderr,"Info: Loading \"%s\" ...\n",bnifile);
	bni = load_bni(fbni);
	if (bni == NULL) return STATUS_FAILURE;
	if (fseek(fbni,bni->dataoffset,SEEK_SET)<0) {
		fprintf(stderr,"%s: could not seek to TGA data offset %lu (fseek: %s)\n",argv[0],(unsigned long int)bni->dataoffset,pstrerror(errno));
		if (bnifile!=dash && fclose(fbni)<0)
			fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
		return STATUS_FAILURE;
	}
	fprintf(stderr,"Info: Loading image ...\n");
	iconimg = load_tga(fbni);
	if (iconimg == NULL) return STATUS_FAILURE;
	
	fprintf(stderr,"Info: Extracting icons ...\n");
	indexfilename = (char*)malloc(strlen(outdir)+14);
	sprintf(indexfilename,"%s/bniindex.lst",outdir);
	fprintf(stderr,"Info: Writing Index to \"%s\" ... \n",indexfilename);
	indexfile = fopen(indexfilename , "w");
	if (indexfile == NULL) {
		fprintf(stderr,"%s: could not open index file \"%s\" for writing (fopen: %s)\n",argv[0],indexfilename,pstrerror(errno));
		if (bnifile!=dash && fclose(fbni)<0)
			fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
		return STATUS_FAILURE;
	}
	fprintf(indexfile,"unknown1 %08x\n",bni->unknown1);
	fprintf(indexfile,"unknown2 %08x\n",bni->unknown2);
	curry = 0;
	for (i=0; i < bni->numicons; i++) {
		FILE *dsttga;
		char *name;
		t_tgaimg *icn;
		icn = area2img(iconimg,0,curry,bni->icons->icon[i].x,bni->icons->icon[i].y,tgaimgtype_uncompressed_truecolor);
		if (icn == NULL) {
			fprintf(stderr,"Error: area2img failed!\n");
			return STATUS_FAILURE;
		}
		if (bni->icons->icon[i].id == 0) {
			int tag = bni->icons->icon[i].tag;
			name = (char*)malloc(strlen(outdir)+10);
			sprintf(name,"%s/%c%c%c%c.tga",outdir,((tag >> 24) & 0xff),((tag >> 16) & 0xff),((tag >> 8) & 0xff),((tag >> 0) & 0xff));
		} else {
			name = (char*)malloc(strlen(outdir)+16);
			sprintf(name,"%s/%08x.tga",outdir,bni->icons->icon[i].id);
		}
		fprintf(stderr,"Info: Writing icon %u(%ux%u) to file \"%s\" ... \n",i+1,icn->width,icn->height,name);
		curry += icn->height;
		dsttga = fopen(name,"w");
		if (dsttga == NULL) {
			fprintf(stderr,"%s: could not open ouptut TGA file \"%s\" for writing (fopen: %s)\n",argv[0],name,pstrerror(errno));
		} else {
			if (write_tga(dsttga,icn) < 0) {
				fprintf(stderr,"Error: Writing to TGA failed.\n");
			} else {
				int tag = bni->icons->icon[i].tag;
				if (bni->icons->icon[i].id == 0) {
					fprintf(indexfile,"icon !%c%c%c%c %d %d %08x\n",((tag >> 24) & 0xff),((tag >> 16) & 0xff),((tag >> 8) & 0xff),((tag >> 0) & 0xff),bni->icons->icon[i].x,bni->icons->icon[i].y,bni->icons->icon[i].unknown);
				} else {
					fprintf(indexfile,"icon #%08x %d %d %08x\n",bni->icons->icon[i].id,bni->icons->icon[i].x,bni->icons->icon[i].y,bni->icons->icon[i].unknown);
				}
			}
			if (fclose(dsttga)<0)
				fprintf(stderr,"%s: could not close TGA file \"%s\" after writing (fclose: %s)\n",argv[0],name,pstrerror(errno));
		}
		free(name);
		destroy_img(icn);
	}
	destroy_img(iconimg);
	if (fclose(indexfile)<0) {
		fprintf(stderr,"%s: could not close index file \"%s\" after writing (fclose: %s)\n",argv[0],indexfilename,pstrerror(errno));
		return STATUS_FAILURE;
	}
	free(indexfilename);
    }
    if (bnifile!=dash && fclose(fbni)<0)
	fprintf(stderr,"%s: could not close BNI file \"%s\" after reading (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
    
    return STATUS_SUCCESS;
}
