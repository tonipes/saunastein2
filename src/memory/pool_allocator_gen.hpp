// Copyright (c) 2025 Inan Evin

#pragma once

#include "pool_handle.hpp"
#include "io/assert.hpp"
#include "memory.hpp"
#include "data/vector.hpp"
#include <new>

namespace SFG
{
	template <typename T, typename SIZE_TYPE, int N> struct pool_allocator_gen
	{
		pool_allocator_gen()
		{
			SFG_MEMSET(_generations, 1, sizeof(SIZE_TYPE) * N);
			SFG_MEMSET(_free_list, 0, sizeof(SIZE_TYPE) * N);
			SFG_MEMSET(_actives, 0, sizeof(uint8) * N);
		}

		inline pool_handle<SIZE_TYPE> add()
		{
			if (_free_count > 0)
			{
				const SIZE_TYPE index = _free_list[_free_count];
				new (&_items[index]) T();
				_free_count--;
				_actives[index] = 1;
				return {
					.generation = _generations[index],
					.index		= index,
				};
			}

			const SIZE_TYPE index = _head;
			_actives[index]		  = 1;
			_head++;
			SFG_ASSERT(_head < N);
			return {
				.generation = _generations[index],
				.index		= index,
			};
		}

		void remove(pool_handle<SIZE_TYPE> handle)
		{
			SFG_ASSERT(is_valid(handle));
			_free_list[_free_count] = handle.index;
			_free_count++;
			_generations[handle.index]++;
			_items[handle.index].~T();
			_actives[handle.index] = 0;
		}

		T& get(pool_handle<SIZE_TYPE> handle)
		{
			SFG_ASSERT(is_valid(handle));
			return _items[handle.index];
		}

		const T& get(pool_handle<SIZE_TYPE> handle) const
		{
			SFG_ASSERT(is_valid(handle));
			return _items[handle.index];
		}

		inline bool is_valid(pool_handle<SIZE_TYPE> handle) const
		{
			return _generations[handle.index] == handle.generation;
		}

		void reset()
		{
			for (int i = 0; i < N; i++)
			{
				if (_actives[i])
					_items[i].~T();
			}

			SFG_MEMSET(_free_list, 0, sizeof(SIZE_TYPE) * N);
			SFG_MEMSET(_actives, 0, sizeof(uint8) * N);
			_head		= 0;
			_free_count = 0;
		}

	private:
		T		  _items[N];
		SIZE_TYPE _free_count = 0;
		SIZE_TYPE _free_list[N];
		SIZE_TYPE _generations[N];
		uint8	  _actives[N];
		SIZE_TYPE _head = 0;
	};

}
