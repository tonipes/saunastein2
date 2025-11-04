// Copyright (c) 2025 Inan Evin

#include "text_allocator.hpp"
#include "memory.hpp"
#include "io/assert.hpp"
#include "data/vector_util.hpp"
#include <cstring>

namespace SFG
{
	void text_allocator::init(uint32 capacity)
	{
		_raw	  = reinterpret_cast<char*>(SFG_MALLOC(capacity));
		_capacity = capacity;
		_free_list.reserve(_capacity / 32);

		if (_raw)
			SFG_MEMSET(_raw, 0, capacity);
	}

	void text_allocator::uninit()
	{
		SFG_FREE(_raw);
		_raw	  = nullptr;
		_capacity = 0;
		_free_list.clear();
	}

	// text_allocator.cpp (only the changed methods shown)

	const char* text_allocator::allocate(size_t len)
	{
		const size_t need = len + 1; // include '\0'

		// find a free block large enough
		auto it = vector_util::find_if(_free_list, [need](const allocation& alloc) { return alloc.size >= need; });

		if (it != _free_list.end())
		{
			allocation& free = *it;

			char* result = free.ptr;
			if (free.size == need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += need;  // advance start of the remaining free block
				free.size -= need; // shrink remaining size
			}

			// ensure there's a terminator so future strlen(ptr) is valid
			result[need - 1] = '\0';
			return result;
		}

		// fallback to bump allocation
		SFG_ASSERT(_head + need <= _capacity); // <= because need already counts the '\0'
		if (_head + need > _capacity)
			return nullptr;

		char* allocated = &_raw[_head];
		_head += need;
		allocated[need - 1] = '\0';
		return allocated;
	}

	const char* text_allocator::allocate(const char* text)
	{
		if (!text)
			return nullptr;

		const size_t need = std::strlen(text) + 1;

		auto it = vector_util::find_if(_free_list, [need](const allocation& alloc) { return alloc.size >= need; });

		if (it != _free_list.end())
		{
			allocation& free = *it;

			char* result = free.ptr;
			if (free.size == need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += need;
				free.size -= need;
			}

			std::memcpy(result, text, need); // copy including '\0'
			return result;
		}

		if (_head + need > _capacity)
			return nullptr;

		char* allocated = &_raw[_head];
		std::memcpy(allocated, text, need);
		_head += need;
		return allocated;
	}

	void text_allocator::deallocate(char* ptr)
	{
		if (!ptr)
			return;
		_free_list.push_back({
			.ptr  = ptr,
			.size = std::strlen(ptr) + 1, // count the '\0' for correct future splits
		});
	}

	void text_allocator::deallocate(const char* ptr)
	{
		if (!ptr)
			return;
		_free_list.push_back({
			.ptr  = const_cast<char*>(ptr),
			.size = std::strlen(ptr) + 1,
		});
	}

}
