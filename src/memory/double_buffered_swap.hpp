// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/atomic.hpp"
#include "common/size_definitions.hpp"

namespace SFG
{
	class alignas(64) double_buffered_swap
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(size_t sz, size_t alignment = 8);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void write(const void* src, size_t padding, size_t n);
		void swap();
		void read(void* dst, size_t n) const;

	private:
		uint8*		  _data[2] = {nullptr};
		atomic<uint8> _index{0};
		size_t		  _sz = 0;
	};
}
