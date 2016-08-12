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
#ifndef INCLUDED_TGA_H
#define INCLUDED_TGA_H

#include <cstdint>
#include <cstdio>


namespace pvpgn
{

	namespace bni
	{

		typedef struct
		{
			std::uint8_t idlen; /* number of bytes in Field 6: 0==no id */
			std::uint8_t cmaptype; /* colormap: 0==none, 1==included, <128==TrueVision, >=128==developer */
			std::uint8_t imgtype; /* pixel format: <128==TrueVision, >=128==developer */
			std::uint16_t cmapfirst; /* first entry offset */
			std::uint16_t cmaplen; /* number of colormap entries */
			std::uint8_t cmapes; /* size of a single colormap entry in bits, 15 forces 1 attribute bit, 32 forces 8  */
			std::uint16_t xorigin; /* x coordinate of lower left hand corner with origin at left of screen */
			std::uint16_t yorigin; /* y coordinate of lower left hand corner with origin at bottom of screen */
			std::uint16_t width; /* width in pixels */
			std::uint16_t height; /* height in pixels */
			std::uint8_t bpp; /* bits per pixel, including attributes and alpha channel */
			std::uint8_t desc; /* image descriptor: bits 0,1,2,3==num attribute bits per pixel, bit 4==horizontal order, bit 5==vertical order, bits 6,7==interleaving */
			/* field 6, optional */
			/* field 7, colormap data in ARGB, optional, entries are (min(cmapes/3,8)*3+7)/8 bits wide */
			std::uint8_t* data;
			/* field 9, developer area, optional */
			/* field 10, extension area, optional */
			std::uint32_t extareaoff; /* extension area offset, 0==none */
			std::uint32_t devareaoff; /* developer area offset, 0==none */
			/* magic, null terminated */
		} t_tgaimg;

		typedef enum
		{
			tgaimgtype_empty = 0,
			tgaimgtype_uncompressed_mapped = 1,
			tgaimgtype_uncompressed_truecolor = 2,
			tgaimgtype_uncompressed_monochrome = 3,
			tgaimgtype_rlecompressed_mapped = 9,
			tgaimgtype_rlecompressed_truecolor = 10,
			tgaimgtype_rlecompressed_monochrome = 11,
			tgaimgtype_huffman_mapped = 32,
			tgaimgtype_huffman_4pass_mapped = 33
		} t_tgaimgtype;

		typedef enum {
			tgadesc_attrbits0 = 1,
			tgadesc_attrbits1 = 2,
			tgadesc_attrbits2 = 4,
			tgadesc_attrbits3 = 8,
			tgadesc_horz = 16,
			tgadesc_vert = 32,
			tgadesc_interleave1 = 64,
			tgadesc_interleave2 = 128
		} t_tgadesc;

		typedef enum {
			tgacmap_none = 0,
			tgacmap_included = 1
		} t_tgacmap;

		typedef enum {
			RLE,
			RAW
		} t_tgapkttype;

		/* "new" style TGA allowing for developer and extension areas must have this magic at
		   the end of the file */
#define TGAMAGIC "TRUEVISION-XFILE."

		extern t_tgaimg * new_tgaimg(unsigned int width, unsigned int height, unsigned int bpp, t_tgaimgtype imgtype);
		extern int getpixelsize(t_tgaimg const *img);
		extern t_tgaimg * load_tgaheader(void);
		extern t_tgaimg * load_tga(std::FILE *f);
		extern int write_tga(std::FILE *f, t_tgaimg *img);
		extern void destroy_img(t_tgaimg * img);
		extern void print_tga_info(t_tgaimg const * img, std::FILE * fp);

	}

}
#endif

