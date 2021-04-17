/*
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_BN_TYPE_TYPES
#define INCLUDED_BN_TYPE_TYPES

#include <cstdint>

namespace pvpgn
{
	using bn_basic = std::uint8_t;
	using bn_byte = bn_basic[1];
	using bn_short = bn_basic[2];
	using bn_int = bn_basic[4];
	using bn_long = bn_basic[8];
}

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BN_TYPE_PROTOS
#define INCLUDED_BN_TYPE_PROTOS

namespace pvpgn
{

	extern int bn_byte_tag_get(bn_byte const * src, char * dst, unsigned int len);
	extern int bn_short_tag_get(bn_short const * src, char * dst, unsigned int len);
	extern int bn_int_tag_get(bn_int const * src, char * dst, unsigned int len);
	extern int bn_long_tag_get(bn_long const * src, char * dst, unsigned int len);

	extern int bn_byte_tag_set(bn_byte * dst, char const * tag);
	extern int bn_short_tag_set(bn_short * dst, char const * tag);
	extern int bn_int_tag_set(bn_int * dst, char const * tag);
	extern int bn_long_tag_set(bn_long * dst, char const * tag);

	extern std::uint8_t bn_byte_get(bn_byte const src);
	extern std::uint16_t bn_short_get(bn_short const src);
	extern std::uint16_t bn_short_nget(bn_short const src);
	extern std::uint32_t bn_int_get(bn_int const src);
	extern std::uint32_t bn_int_nget(bn_int const src);
	extern std::uint64_t bn_long_get(bn_long const src);
	extern std::uint32_t bn_long_get_a(bn_long const src);
	extern std::uint32_t bn_long_get_b(bn_long const src);

	extern int bn_byte_set(bn_byte * dst, std::uint8_t src);
	extern int bn_short_set(bn_short * dst, std::uint16_t src);
	extern int bn_short_nset(bn_short * dst, std::uint16_t src);
	extern int bn_int_set(bn_int * dst, std::uint32_t src);
	extern int bn_int_nset(bn_int * dst, std::uint32_t src);
	extern int bn_long_set(bn_long * dst, std::uint64_t src);
	extern int bn_long_nset(bn_long * dst, std::uint64_t src);
	extern int bn_long_set_a_b(bn_long * dst, std::uint32_t srca, std::uint32_t srcb);
	extern int bn_long_nset_a_b(bn_long * dst, std::uint32_t srca, std::uint32_t srcb);

	extern int bn_raw_set(void * dst, void const * src, unsigned int len);

	extern int bn_byte_tag_eq(bn_byte const src, char const * tag);
	extern int bn_short_tag_eq(bn_short const src, char const * tag);
	extern int bn_int_tag_eq(bn_int const src, char const * tag);
	extern int bn_long_tag_eq(bn_long const src, char const * tag);

	extern int uint32_to_int(std::uint32_t num);

}

#endif
#endif
