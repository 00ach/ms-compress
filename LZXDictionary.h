// ms-compress: implements Microsoft compression algorithms
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


/////////////////// Dictionary /////////////////////////////////////////////////
// The dictionary system used for LZX compression.
//
// TODO: ? Most of the compression time is spent in the dictionary - particularly Find and Add.

#ifndef LZX_DICTIONARY_H
#define LZX_DICTIONARY_H
#include "compression-api.h"

#ifdef __cplusplus_cli
#pragma unmanaged
#endif

#if defined(_MSC_VER) && defined(NDEBUG)
#pragma optimize("t", on)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4512) // warning C4512: assignment operator could not be generated
#endif

#include "LCG.h"
#include "LZXConstants.h"

class LZXDictionary
{
private:
	static const uint16_t MaxHash = 0x8000;

	// Define a LCG-generate hash table
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4309) // warning C4309: 'specialization' : truncation of constant value
#endif
	typedef LCG<0x2a190348ul, 0x41C64E6Du, 12345u, 1ull << 32, 16, MaxHash> lcg;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	const uint32_t windowSize;
	const_bytes start, end, end3;
#ifdef LARGE_STACK
	const_bytes table[MaxHash];
#else
	const_bytes* table;
#endif
	const_bytes* window;

	inline uint32_t WindowPos(const_bytes x) const { return (uint32_t)((x - this->start) & (windowSize - 1)); } // { return (uint32_t)((x - this->start) % (2 * settings->WindowSize)); }
	inline static uint32_t GetMatchLength(const_bytes a, const_bytes b, const const_bytes end, const const_bytes end4)
	{
		// like memcmp but tells you the length of the match and optimized
		// assumptions: a < b < end, end4 = end - 4
		const const_bytes b_start = b;
		byte a0, b0;
		while (b < end4 && *((uint32_t*)a) == *((uint32_t*)b))
		{
			a += sizeof(uint32_t);
			b += sizeof(uint32_t);
		}
		do
		{
			a0 = *a++;
			b0 = *b++;
		} while (b < end && a0 == b0);
		return (uint32_t)(b - b_start - 1);
	}

public:
	inline LZXDictionary(const const_bytes start, const uint32_t windowSize) : start(start), end(start + windowSize), end3(end - 3), windowSize(windowSize)
	{
#ifndef LARGE_STACK
		this->table  = (const_bytes*)malloc(MaxHash     *sizeof(const_bytes));
#endif
		this->window = (const_bytes*)malloc(windowSize*2*sizeof(const_bytes));
		memset(this->table,  0, MaxHash     *sizeof(const_bytes));
		memset(this->window, 0, windowSize*2*sizeof(const_bytes));
	}
	inline ~LZXDictionary()
	{
#ifndef LARGE_STACK
		free(this->table);
#endif
		free(this->window);
	}

	inline void Add(const_bytes data, size_t len)
	{
		uint32_t pos = WindowPos(data);
		const const_bytes end = ((data + len) < this->end3) ? data + len : this->end3;
		while (data < end)
		{
			const uint32_t hash = lcg::Hash(data);
			this->window[pos++] = this->table[hash];
			this->table[hash] = data++;
		}
	}

	inline uint32_t Find(const const_bytes data, uint32_t* offset) const
	{
		const const_bytes end = ((data + 257) >= this->end) ? this->end : data + 257; // if overflow or past end use the end, otherwise go MaxLength away
		const const_bytes xend = data - windowSize - 3, end4 = end - 4;
		const_bytes x;
		uint32_t len = 1;
		for (x = this->window[WindowPos(data)]; x >= xend; x = this->window[WindowPos(x)])
		{
			const uint32_t l = GetMatchLength(x, data, end, end4);
			if (l > len)
			{
				*offset = (uint32_t)(data - x);
				len = l;
			}
		}
		return len;
	}
};

#endif