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
#ifndef INCLUDED_CONFFILE_H
#define INCLUDED_CONFFILE_H

typedef enum
{
	conf_type_none,
	conf_type_bool,
	conf_type_int,
	conf_type_str,
	conf_type_hexstr
} e_conf_type;

typedef struct
{
	char const	* name;
	int		offset;
	e_conf_type	type;
	int		def_intval;
	char const	* def_strval;
} t_conf_table;

extern int conf_cleanup(t_conf_table * conf_table, void * param_data, int size);
extern int conf_load_file(char const * filename, t_conf_table * conf_table, void * data, int datalen);
extern int conf_parse_param(int argc, char * * argv, t_conf_table * conf_table, void * data, int datalen);

#endif
