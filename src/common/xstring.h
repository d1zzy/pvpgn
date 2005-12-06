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
#ifndef INCLUDED_XSTRING_H
#define INCLUDED_XSTRING_H

extern char *		strtolower(char * str);
extern char *		hexstrdup(unsigned char const * src);
extern unsigned int	hexstrtoraw(unsigned char const * src, char * data, unsigned int datalen);
extern unsigned char	xtoi(unsigned char ch);
extern char * *		strtoargv(char const * str, unsigned int * count);
extern char *		arraytostr(char * * array, char const * delim, int count);
extern char *		str_strip_affix(char * str, char const * affix);

#endif
