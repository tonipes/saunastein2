// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/atomic.hpp"
#include "memory/memory.hpp"
#include "common/size_definitions.hpp"

namespace SFG
{
	template <int SIZE> class alignas(64) double_buffered_swap
	{
	public:
		inline void write(const void* src, size_t padding, size_t n)
		{
			if (padding + n > SIZE)
				n = SIZE;

			uint8_t cur	 = _index.load(std::memory_order_relaxed);
			uint8_t next = cur ^ 1;
			SFG_MEMCPY(_data[next] + padding, src, n);
			_index.store(next, std::memory_order_release);
		}

		inline void read(void* dst, size_t n) const
		{
			if (n > SIZE)
				n = SIZE;

			uint8_t cur = _index.load(std::memory_order_acquire);
			std::memcpy(dst, _data[cur], n);
		}

	private:
		uint8		  _data[2][SIZE];
		atomic<uint8> _index{0};
	};
}
