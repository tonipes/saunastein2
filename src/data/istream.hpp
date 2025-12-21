/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common/size_definitions.hpp"
#include "serialization/endianness.hpp"
#include "memory/memory.hpp"
#include "data/string.hpp"
#include "common/types.hpp"
#include "io/assert.hpp"
#include <fstream>

namespace SFG
{


	template <typename Stream, typename U> void deserialize_vector(Stream& stream, vector<U>& vec)
	{
		uint32 sz = 0;
		stream >> sz;
		vec.resize(sz);
		if constexpr (std::is_pointer<U>::value)
		{
			for (uint32 i = 0; i < sz; i++)
			{
				using UnderlyingType = typename std::remove_pointer<U>::type;
				vec[i]				 = new UnderlyingType();
				vec[i]->deserialize(stream);
			}
		}
		else
		{
			for (uint32 i = 0; i < sz; i++)
				stream >> vec[i];
		}
	}

	class istream
	{
	public:
		istream() {};
		istream(uint8* data, size_t size)
		{
			_data = data;
			_size = size;
		}
		void open(uint8* data, size_t size);
		void close();
		void create(uint8* data, size_t size);
		void destroy();
		void read_from_ifstream(std::ifstream& stream);
		void read_to_raw_endian_safe(void* ptr, size_t size);
		void read_to_raw(uint8* ptr, size_t size);

		template <typename T> void read(T& t)
		{
			SFG_MEMCPY(reinterpret_cast<uint8*>(&t), &_data[_index], sizeof(T));
			_index += sizeof(T);
		}

		inline void skip_by(size_t size)
		{
			_index += size;
		}

		inline void seek(size_t ind)
		{
			_index = ind;
		}

		inline size_t get_size() const
		{
			return _size;
		}

		inline bool empty() const
		{
			return _size == 0;
		}

		inline uint8* get_raw() const
		{
			return _data;
		}

		inline uint8* get_data_current()
		{
			return &_data[_index];
		}

		inline void shrink(size_t size)
		{
			_size = size;
		}

		inline size_t tellg() const
		{
			return _index;
		}

		inline bool is_eof() const
		{
			return _index >= _size;
		}

	private:
		uint8* _data  = nullptr;
		size_t _index = 0;
		size_t _size  = 0;
	};

	template <typename T> istream& operator>>(istream& stream, T& val)
	{
		if constexpr (std::is_arithmetic_v<T>)
		{
			stream.read(val);
			if (endianness::should_swap())
				endianness::swap_endian(val);
		}
		else if constexpr (std::is_same_v<T, string>)
		{
			uint32 sz = 0;
			stream >> sz;
			val = string((char*)stream.get_data_current(), sz);
			stream.skip_by(sz);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			uint8 u8 = 0;
			stream >> u8;
			val = static_cast<T>(u8);
		}
		else if constexpr (is_vector_v<T>)
		{
			deserialize_vector(stream, val);
		}
		else if constexpr (std::is_class_v<T>)
		{
			// Handle custom classes or structs
			val.deserialize(stream);
		}
		else
		{
			SFG_ASSERT(false, "");
		}

		return stream;
	}

}
