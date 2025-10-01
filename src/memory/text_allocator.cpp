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
		SFG_MEMSET(_raw, 0, capacity);
	}

	void text_allocator::uninit()
	{
		SFG_FREE(_raw);
		_raw	  = nullptr;
		_capacity = 0;
		_free_list.clear();
	}

	const char* text_allocator::allocate(size_t len)
	{
		auto it = vector_util::find_if(_free_list, [len](const allocation& alloc) -> bool { return alloc.size > len; });
		if (it != _free_list.end())
		{
			allocation& free = *it;
			if (free.size == len)
			{
				_free_list.erase(it);
				return free.ptr;
			}
			else
			{
				free.size -= len;
				return free.ptr;
			}
		}

		SFG_ASSERT(_head + len + 1 < _capacity);

		char* allocated = &_raw[_head];
		_head += len + 1;

		return allocated;
	}

	const char* text_allocator::allocate(const char* text)
	{
		if (!text)
			return nullptr;

		const size_t len = strlen(text);

		auto it = vector_util::find_if(_free_list, [len](const allocation& alloc) -> bool { return alloc.size > len; });
		if (it != _free_list.end())
		{
			allocation& free = *it;
			if (free.size == len)
			{
				std::strcpy(free.ptr, text);
				_free_list.erase(it);
				return free.ptr;
			}
			else
			{
				free.size -= len;
				std::strcpy(free.ptr, text);
				return free.ptr;
			}
		}

		SFG_ASSERT(_head + len + 1 < _capacity);

		char* allocated = &_raw[_head];
		std::strcpy(allocated, text);
		_head += len + 1;

		return allocated;
	}

	void text_allocator::deallocate(char* ptr)
	{
		_free_list.push_back({
			.ptr  = ptr,
			.size = strlen(ptr),
		});
	}

	void text_allocator::deallocate(const char* ptr)
	{
		_free_list.push_back({
			.ptr  = (char*)ptr,
			.size = strlen(ptr),
		});
	}
}
