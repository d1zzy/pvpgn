/*
 * Copyright (C) 2003  Aaron
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
#ifndef INCLUDED_BINARY_LADDER_TYPES
#define INCLUDED_BINARY_LADDER_TYPES

namespace pvpgn
{

typedef enum
{	WAR3_SOLO, WAR3_TEAM, WAR3_FFA, WAR3_AT,
	W3XP_SOLO, W3XP_TEAM, W3XP_FFA, W3XP_AT,
	STAR_AR,   STAR_AW,   STAR_AG,	/* AR = active-rating, AW = active-wins, AG = active-games */
	STAR_CR,   STAR_CW,   STAR_CG,	/* CR = current-rating, CW = current-wins, CG = current-games */
	SEXP_AR,   SEXP_AW,   SEXP_AG,
	SEXP_CR,   SEXP_CW,   SEXP_CG,
	W2BN_AR,   W2BN_AW,   W2BN_AG,
	W2BN_CR,   W2BN_CW,   W2BN_CG,
	W2BN_ARI,  W2BN_AWI,  W2BN_AGI, /* I = Ironman */
	W2BN_CRI,  W2BN_CWI,  W2BN_CGI

} t_binary_ladder_types;

typedef enum
{	load_success = 0,
        illegal_checksum,
	load_failed
} t_binary_ladder_load_result;

typedef int (* t_cb_get_from_ladder)(t_binary_ladder_types type, int rank, int *results);
typedef int (* t_cb_add_to_ladder)(t_binary_ladder_types, int *values);

}

#ifdef BINARY_LADDER_INTERNAL_ACCESS

#define magick 0xdeadbeef

#endif

#endif

/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BINARY_LADDER_PROTOS
#define INCLUDED_BINARY_LADDER_PROTOS

// some protos here
namespace pvpgn
{

extern int binary_ladder_save(t_binary_ladder_types type, unsigned int paracount, t_cb_get_from_ladder _cb_get_from_ladder);
extern t_binary_ladder_load_result binary_ladder_load(t_binary_ladder_types type, unsigned int paracount, t_cb_add_to_ladder _cb_add_to_ladder);

}

#endif
#endif
