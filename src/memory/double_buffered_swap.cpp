// Copyright (c) 2025 Inan Evin

#include "double_buffered_swap.hpp"
#include "io/assert.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"

namespace SFG
{
	void double_buffered_swap::init(size_t sz, size_t alignment)
	{
		_data[0] = reinterpret_cast<uint8*>(SFG_ALIGNED_MALLOC(alignment, sz));
		_data[1] = reinterpret_cast<uint8*>(SFG_ALIGNED_MALLOC(alignment, sz));
		_sz		 = sz;
		PUSH_ALLOCATION_SZ(sz * 2);
	}

	void double_buffered_swap::uninit()
	{
		SFG_ALIGNED_FREE(_data[0]);
		SFG_ALIGNED_FREE(_data[1]);
		PUSH_DEALLOCATION_SZ(_sz * 2);
	}

	void double_buffered_swap::write(const void* src, size_t padding, size_t n)
	{
		SFG_ASSERT(padding + n < _sz);

		uint8_t cur = _index.load(std::memory_order_relaxed);
		SFG_MEMCPY(_data[cur] + padding, src, n);
	}

	void double_buffered_swap::swap()
	{
		uint8_t cur	 = _index.load(std::memory_order_relaxed);
		uint8_t next = cur ^ 1;
		_index.store(next, std::memory_order_release);
	}

	void double_buffered_swap::read(void* dst, size_t n) const
	{
		SFG_ASSERT(n < _sz);

		uint8_t cur = _index.load(std::memory_order_acquire);
		std::memcpy(dst, _data[cur], n);
	}
}
