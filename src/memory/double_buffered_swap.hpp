// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/atomic.hpp"
#include "memory/memory.hpp"
#include "common/size_definitions.hpp"
#include "io/assert.hpp"

namespace SFG
{
	template <int SIZE> class alignas(64) double_buffered_swap
	{
	public:
		inline void write(const void* src, size_t padding, size_t n)
		{
			SFG_ASSERT(padding + n < SIZE);

			uint8_t cur	 = _index.load(std::memory_order_relaxed);
			SFG_MEMCPY(_data[cur] + padding, src, n);
		}

		inline void swap()
		{
			uint8_t cur	 = _index.load(std::memory_order_relaxed);
			uint8_t next = cur ^ 1;
			_index.store(next, std::memory_order_release);
		}

		inline void read(void* dst, size_t n) const
		{
			SFG_ASSERT(n < SIZE);

			uint8_t cur = _index.load(std::memory_order_acquire);
			std::memcpy(dst, _data[cur], n);
		}

	private:
		uint8		  _data[2][SIZE];
		atomic<uint8> _index{0};
	};
}
