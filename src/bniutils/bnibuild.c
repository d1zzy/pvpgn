/*
    Copyright (C) 2000  Marco Ziech
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
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include "compat/statmacros.h"
#include "fileio.h"
#include "tga.h"
#include "bni.h"
#include "common/version.h"
#include "common/setup_after.h"


#define BUFSIZE 1024


static int read_list(char const * progname, t_bnifile * bnifile, char const * name) {
	FILE * f;
	char   line[BUFSIZE];
	
	f = fopen(name,"r");
	if (f == NULL) {
		fprintf(stderr,"%s: could not open index file \"%s\" for reading (fopen: %s)\n",progname,name,pstrerror(errno));
		return -1;
	}
	bnifile->unknown1 = 0x00000010; /* in case they are not set */
	bnifile->unknown2 = 0x00000001;
	bnifile->numicons = 0; 
	bnifile->dataoffset = 16; /* size of header */
	bnifile->icons = (struct bni_iconlist_struct*)malloc(1); /* some realloc()s are broken */
	while (fgets(line,sizeof(line),f)) {
		char cmd[BUFSIZE];
		sscanf(line,"%s",cmd);
		if (strcmp(cmd,"unknown1") == 0) {
			sscanf(line,"unknown1 %08x",&bnifile->unknown1);
		}
		else if (strcmp(cmd,"unknown2") == 0) {
			sscanf(line,"unknown2 %08x",&bnifile->unknown2);
		}
		else if (strcmp(cmd,"icon") == 0) {
			char c;
			sscanf(line,"icon %c",&c);
			if (c == '!') {
				unsigned char tg[4];
				int tag;
				unsigned int x,y,unknown;
				sscanf(line,"icon !%c%c%c%c %u %u %08x",&tg[0],&tg[1],&tg[2],&tg[3],&x,&y,&unknown);
				tag = tg[3] + (tg[2] << 8) + (tg[1] << 16) + (tg[0] << 24);
				fprintf(stderr,"Icon[%d]: id=0x%x x=%u y=%u unknown=0x%x tag=\"%c%c%c%c\"\n",bnifile->numicons,0,x,y,unknown,((tag >> 24) & 0xff),((tag >> 16) & 0xff),((tag >> 8) & 0xff),((tag >> 0) & 0xff));
				bnifile->icons = (struct bni_iconlist_struct*)realloc(bnifile->icons,((bnifile->numicons+1)*sizeof(t_bniicon)));
				bnifile->icons->icon[bnifile->numicons].id = 0;
				bnifile->icons->icon[bnifile->numicons].x = x;
				bnifile->icons->icon[bnifile->numicons].y = y;
				bnifile->icons->icon[bnifile->numicons].tag = tag;
				bnifile->icons->icon[bnifile->numicons].unknown = unknown;
				bnifile->numicons++;
				bnifile->dataoffset += 20;
			}
			else if (c == '#') {
				unsigned int id,x,y,unknown;
				sscanf(line,"icon #%08x %u %u %08x",&id,&x,&y,&unknown);
				fprintf(stderr,"Icon[%d]: id=0x%x x=%u y=%u unknown=0x%x tag=0x00000000\n",bnifile->numicons,id,x,y,unknown);
				bnifile->icons = (struct bni_iconlist_struct*)realloc(bnifile->icons,((bnifile->numicons+1)*sizeof(t_bniicon)));
				bnifile->icons->icon[bnifile->numicons].id=id;
				bnifile->icons->icon[bnifile->numicons].x = x;
				bnifile->icons->icon[bnifile->numicons].y = y;
				bnifile->icons->icon[bnifile->numicons].tag=0;
				bnifile->icons->icon[bnifile->numicons].unknown = unknown;
				bnifile->numicons++;
				bnifile->dataoffset += 16;
			}
			else
				fprintf(stderr,"Bad character '%c' in icon specifier for icon %u in index file \"%s\"\n",c,bnifile->numicons+1,name);
		}
		else
			fprintf(stderr,"Unknown command \"%s\" in index file \"%s\"\n",cmd,name);
	}
	if (fclose(f)<0)
		fprintf(stderr,"%s: could not close index file \"%s\" after reading (fclose: %s)\n",progname,name,pstrerror(errno));
	return 0;
}


static char * geticonfilename(t_bnifile *bnifile, char const * indir, int i) {
	char * name;

	if (bnifile->icons->icon[i].id == 0) {
		unsigned int tag = bnifile->icons->icon[i].tag;
		name = (char*)malloc(strlen(indir)+10);
		sprintf(name,"%s/%c%c%c%c.tga",indir,((tag >> 24) & 0xff),((tag >> 16) & 0xff),((tag >> 8) & 0xff),((tag >> 0) & 0xff));
	} else {
		name = (char*)malloc(strlen(indir)+16);
		sprintf(name,"%s/%08x.tga",indir,bnifile->icons->icon[i].id);
	}
	return name;
}


static int img2area(t_tgaimg *dst, t_tgaimg *src, int x, int y) {
	unsigned char *sdp;
	unsigned char *ddp;
	int pixelsize;
	int i;

	pixelsize = getpixelsize(dst);
	if (getpixelsize(src) != pixelsize) {
		fprintf(stderr,"Error: source pixelsize is %d should be %d!\n",getpixelsize(src),pixelsize);
		return -1;
	}
	if (src->width+x > dst->width) return -1;
	if (src->height+y > dst->height) return -1;
	sdp = src->data;
	ddp = dst->data + (y * dst->width * pixelsize);
	for (i = 0; i < src->height; i++) {
		ddp += x*pixelsize;
		memcpy(ddp,sdp,src->width*pixelsize);
		sdp += src->width*pixelsize;
		ddp += (dst->width-x)*pixelsize;
	}
	return 0;
}


static void usage(char const * progname)
{
    fprintf(stderr,
            "usage: %s [<options>] [--] <input directory> [<BNI file>]\n"
            "    -h, --help, --usage  show this information and exit\n"
            "    -v, --version        print version number and exit\n",progname);

    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    char const * indir=NULL;
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
        if (forcefile && !indir)
            indir = argv[a];
        else if (strcmp(argv[a],"-")==0 && !indir)
            indir = dash;
        else if (argv[a][0]!='-' && !indir)
            indir = argv[a];
        else if (forcefile && !bnifile)
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
    
    if (!indir)
    {
        fprintf(stderr,"%s: input directory not specified\n",argv[0]);
        usage(argv[0]);
    }
    if (!bnifile)
	bnifile = dash;
    
    if (indir==dash)
    {
	fprintf(stderr,"%s: can not read directory from <stdin>\n",argv[0]);
	return STATUS_FAILURE;
    }
    if (stat(indir,&s)<0) {
        fprintf(stderr,"%s: could not stat input directory \"%s\" (stat: %s)\n",argv[0],indir,pstrerror(errno));
	return STATUS_FAILURE;
    }
    if (!S_ISDIR(s.st_mode)) {
        fprintf(stderr,"%s: \"%s\" is not a directory\n",argv[0],indir);
	return -1;
    }
    
    if (bnifile==dash)
	fbni = stdout;
    else
	if (!(fbni = fopen(bnifile,"w")))
	{
	    fprintf(stderr,"%s: could not open BNI file \"%s\" for writing (fopen: %s)\n",argv[0],bnifile,pstrerror(errno));
	    return STATUS_FAILURE;
	}
    
    {
	unsigned int i;
	unsigned int yline;
	t_tgaimg *   img;
	t_bnifile    bni;
	char *       listfilename;
	
	listfilename = (char*)malloc(strlen(indir)+14);
	sprintf(listfilename,"%s/bniindex.lst",indir);
	fprintf(stderr,"Info: Reading index from file \"%s\"...\n",listfilename);
	if (read_list(argv[0],&bni,listfilename)<0)
	    return STATUS_FAILURE;
	fprintf(stderr,"BNIHeader: unknown1=%u unknown2=%u numicons=%u dataoffset=%u\n",bni.unknown1,bni.unknown2,bni.numicons,bni.dataoffset);
	if (write_bni(fbni,&bni)<0) {
		fprintf(stderr,"Error: Failed to write BNI header.\n");
		return STATUS_FAILURE;
	}
	img = new_tgaimg(0,0,24,tgaimgtype_rlecompressed_truecolor);
	for (i = 0; i < bni.numicons; i++) {
		if (bni.icons->icon[i].x > img->width) img->width = bni.icons->icon[i].x;
		img->height += bni.icons->icon[i].y;
	}
	fprintf(stderr,"Info: Creating TGA with %ux%ux%ubpp.\n",img->width,img->height,img->bpp);
	img->data = (t_uint8*)malloc(img->width*img->height*getpixelsize(img));
	yline = 0;
	for (i = 0; i < bni.numicons; i++) {
		t_tgaimg *icon;
		FILE *f;
		char *name;
		name = geticonfilename(&bni,indir,i);
		f = fopen(name,"r");
		if (f==NULL) {
			perror("fopen");
			free(name);
			return STATUS_FAILURE;
		}
		free(name);
		icon = load_tga(f);
		if (fclose(f)<0)
			fprintf(stderr,"Error: could not close TGA file \"%s\" after reading (fclose: %s)\n",name,pstrerror(errno));
		if (icon == NULL) {
			fprintf(stderr,"Error: load_tga failed with data from TGA file \"%s\"\n",name);
			return STATUS_FAILURE;			
		}
		if (img2area(img,icon,0,yline)<0) {
			fprintf(stderr,"Error: inserting icon from TGA file \"%s\" into big TGA failed\n",name);
			return STATUS_FAILURE;
		}
		yline += icon->height;
		destroy_img(icon);
	}	
	if (write_tga(fbni,img)<0) {
		fprintf(stderr,"Error: Failed to write TGA to BNI file.\n");
		return STATUS_FAILURE;
	}
	if (bnifile!=dash && fclose(fbni)<0) {
		fprintf(stderr,"%s: could not close BNI file \"%s\" after writing (fclose: %s)\n",argv[0],bnifile,pstrerror(errno));
		return STATUS_FAILURE;
	}
    }
    fprintf(stderr,"Info: Writing to \"%s\" finished.\n",bnifile);
    return STATUS_SUCCESS;
}
