/*
 * Class for simple, arbitrary size unsigned integer math.
 * Note that some method implementations might lack features that weren't required so far!
 *
 * Copyright (C) 2008 - Olaf Freyer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 */

#ifndef INCLUDED_BIGINT_H
#define INCLUDED_BIGINT_H

#include <cstdint>
#include <string>

namespace pvpgn
{

	class BigInt
	{
	public:
		BigInt();
		explicit BigInt(std::uint8_t input);
		explicit BigInt(std::uint16_t input);
		explicit BigInt(std::uint32_t input);
		BigInt(const BigInt& input);
		BigInt& operator=(const BigInt& input);
		BigInt(unsigned char const* input, int input_size, int blockSize = 1, bool bigEndian = true);
		~BigInt() throw ();
		bool operator== (const BigInt& right) const;
		bool operator<  (const BigInt& right) const;
		bool operator>  (const BigInt& right) const;
		BigInt operator+ (const BigInt& right) const;
		BigInt operator- (const BigInt& right) const;
		BigInt operator* (const BigInt& right) const;
		BigInt operator/ (const BigInt& right) const;
		BigInt operator% (const BigInt& right) const;
		BigInt operator<< (int bytesToShift) const;
		static BigInt random(int size);
		static void status();
		BigInt powm(const BigInt& exp, const BigInt& mod) const;
		unsigned char* getData(int byteCount, int blockSize = 1, bool bigEndian = true) const;
		void getData(unsigned char* out, int byteCount, int blockSize = 1, bool bigEndian = true) const;
		std::string toHexString() const;

	private:
		
		explicit BigInt(std::uint64_t input);
		using bigint_base = std::uint32_t;
		using bigint_extended = std::uint64_t;
#define bigint_base_mask 0xffffffff
#define bigint_extended_carry bigint_base_mask+0x01
#define bigint_base_bitcount (sizeof(bigint_base)*8)

		bigint_base	*segment;
		int 		segment_count;
	};

}

#endif /* __BIGINT_INCLUDED__ */
