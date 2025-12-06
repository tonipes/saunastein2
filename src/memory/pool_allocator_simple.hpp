// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "io/assert.hpp"
#include "memory.hpp"
#include "data/vector.hpp"

namespace SFG
{
	template <typename T, int N, typename SIZE_TYPE = uint32> struct pool_allocator_simple
	{
		~pool_allocator_simple()
		{
		}

		pool_allocator_simple()
		{
		}

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------
		inline void reset()
		{
			for (SIZE_TYPE i = 0; i < N; i++)
				_items[i] = T();
		}

		inline void reset(SIZE_TYPE idx)
		{
			SFG_ASSERT(idx < N);
			_items[idx] = T();
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		T& get(SIZE_TYPE idx)
		{
			SFG_ASSERT(idx < N);
			return _items[idx];
		}

		const T& get(SIZE_TYPE idx) const
		{
			SFG_ASSERT(idx < N);
			return _items[idx];
		}

		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		template <typename TYPE> struct iterator
		{
			using reference = TYPE&;
			using pointer	= TYPE*;

			iterator(pointer ptr, SIZE_TYPE begin, SIZE_TYPE end) : _ptr(ptr), _current(begin), _end(end)
			{
			}

			reference operator*() const
			{
				return *_ptr;
			};
			pointer operator->()
			{
				return _ptr;
			}

			iterator& operator++()
			{
				_current++;
				return *this;
			}

			iterator& operator++(int)
			{
				iterator tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const iterator& a, const iterator& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const iterator& a, const iterator& b)
			{
				return a._current != b._current;
			}

			pointer	  _ptr	   = nullptr;
			SIZE_TYPE _current = 0;
			SIZE_TYPE _end	   = 0;
		};

		iterator<const T> begin() const
		{
			return iterator<const T>(_items, 0, N);
		}

		iterator<const T> end() const
		{
			return iterator<const T>(_items, N, N);
		}

		iterator<T> begin()
		{
			return iterator<T>(_items, 0, N);
		}

		iterator<T> end()
		{
			return iterator<T>(_items, N, N);
		}

	private:
		T _items[N];
	};

}
