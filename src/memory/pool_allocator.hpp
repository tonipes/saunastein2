// Copyright (c) 2025 Inan Evin

#pragma once

#include "pool_handle.hpp"
#include "io/assert.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"

namespace SFG
{
	template <typename SIZE_TYPE> class pool_allocator
	{
	public:
		~pool_allocator()
		{
			if (_raw != nullptr)
				uninit();
		}

		template <typename T> inline void init(size_t item_count)
		{
			const size_t item_size		= sizeof(T);
			const size_t item_alignment = alignof(T);
			SFG_ASSERT(_raw == nullptr);
			SFG_ASSERT(item_size % item_alignment == 0);

			auto align = [](size_t sz, size_t alignment) -> size_t { return (sz + alignment + 1) & ~(alignment - 1); };

			const size_t padded_item_size = align(item_size, item_alignment);
			const size_t sz_items		  = padded_item_size * item_count;
			_item_size_aligned			  = padded_item_size;

			const size_t sz_free_indices = align(sizeof(SIZE_TYPE) * item_count, alignof(SIZE_TYPE));
			const size_t sz_generations	 = align(sizeof(SIZE_TYPE) * item_count, alignof(SIZE_TYPE));
			const size_t sz_actives		 = align(sizeof(uint8) * item_count, alignof(SIZE_TYPE));
			const size_t total_size		 = sz_items + sz_free_indices + sz_generations + sz_actives;
			_raw						 = reinterpret_cast<uint8*>(SFG_ALIGNED_MALLOC(item_alignment, total_size));

#ifdef ENABLE_MEMORY_TRACER
			memory_tracer::get().on_allocation(_raw, total_size);
#endif
			_item_count = static_cast<SIZE_TYPE>(item_count);

			if (_raw != nullptr)
			{
				T* ptr = reinterpret_cast<T*>(_raw);

				SIZE_TYPE* generations	= get_generations();
				SIZE_TYPE* free_indices = get_free_indices();
				uint8*	   actives		= get_actives();

				for (SIZE_TYPE i = 0; i < _item_count; i++)
				{
					free_indices[i] = 0;
					generations[i]	= 1;
					actives[i]		= 0;
					ptr[i]			= T();
				}
			}
		}

		inline void uninit()
		{
#ifdef ENABLE_MEMORY_TRACER
			memory_tracer::get().on_free(_raw);
#endif
			SFG_ASSERT(_raw != nullptr);
			reset();
			SFG_ALIGNED_FREE(_raw);
			_raw = nullptr;
		}

		void reset()
		{
			_head		= 0;
			_free_count = 0;

			SIZE_TYPE* free_indices = get_free_indices();
			uint8*	   actives		= get_actives();

			for (SIZE_TYPE i = 0; i < _item_count; i++)
			{
				free_indices[i] = 0;
				actives[i]		= 0;
			}
		}

		bool is_valid(pool_handle16 handle) const
		{
			SIZE_TYPE* generations = get_generations();
			return handle.generation == generations[handle.index];
		}

		template <typename T> inline void free(pool_handle16 handle)
		{
			SFG_ASSERT(is_valid(handle));

			T& t = get<T>(handle);
			t.~T();

			SIZE_TYPE* generations	= get_generations();
			SIZE_TYPE* free_indices = get_free_indices();
			uint8*	   actives		= get_actives();

			actives[handle.index] = 0;
			generations[handle.index]++;
			free_indices[_free_count] = handle.index;
			_free_count++;
		}

		template <typename T> pool_handle16 allocate()
		{
			const size_t aligned = (sizeof(T) + alignof(T) + 1) & ~(alignof(T) - 1);
			SFG_ASSERT(aligned == static_cast<size_t>(_item_size_aligned));

			SIZE_TYPE index = 0;

			if (_free_count != 0)
			{
				SIZE_TYPE* free_indices = get_free_indices();
				index					= free_indices[_free_count - 1];
				_free_count--;
			}
			else
			{
				index = _head;
				_head++;
				SFG_ASSERT(_head <= _item_count);
			}

			uint8* actives = get_actives();
			actives[index] = 1;

			T& data = *(reinterpret_cast<T*>(_raw + static_cast<size_t>(_item_size_aligned) * index));
			data	= T();

			SIZE_TYPE* generations = get_generations();
			return {.generation = generations[index], .index = index};
		}

		template <typename T> inline T& get(pool_handle16 handle) const
		{
			SFG_ASSERT(is_valid(handle));
			const size_t pos  = sizeof(T) * handle.index;
			T*			 item = reinterpret_cast<T*>(_raw + pos);
			return *item;
		}

		struct handle_iterator
		{
			SIZE_TYPE* gens	   = nullptr;
			uint8*	   actives = nullptr;
			SIZE_TYPE  current = 0;
			SIZE_TYPE  end	   = 0;

			handle_iterator(SIZE_TYPE _start, SIZE_TYPE _end, SIZE_TYPE* _gens, uint8* _actives) : current(_start), gens(_gens), actives(_actives), end(_end)
			{
				while (current != end && actives[current] == 0)
					++current;
			}

			pool_handle16 operator*() const
			{
				return {.generation = gens[current], .index = current};
			}

			handle_iterator& operator++()
			{
				do
				{
					++current;
				} while (current != end && actives[current] == 0);
				return *this;
			}

			bool operator==(const handle_iterator& other) const
			{
				return current == other.current;
			}

			bool operator!=(const handle_iterator& other) const
			{
				return current != other.current;
			}
		};

		handle_iterator begin() const
		{
			return handle_iterator(0, _head, get_generations(), get_actives());
		}

		handle_iterator end() const
		{
			return handle_iterator(_head, _head, get_generations(), get_actives());
		}

		inline uint8* get_raw() const
		{
			return _raw;
		}

	private:
		SIZE_TYPE* get_free_indices() const
		{
			return reinterpret_cast<SIZE_TYPE*>(_raw + _item_count * _item_size_aligned + sizeof(SIZE_TYPE) * _item_count);
		}
		SIZE_TYPE* get_generations() const
		{
			return reinterpret_cast<SIZE_TYPE*>(_raw + _item_count * _item_size_aligned);
		}
		uint8* get_actives() const
		{
			return reinterpret_cast<uint8*>(_raw + _item_count * _item_size_aligned + sizeof(SIZE_TYPE) * _item_count + sizeof(SIZE_TYPE) * _item_count);
		}

	private:
		uint8*	  _raw				 = nullptr;
		SIZE_TYPE _head				 = 0;
		SIZE_TYPE _free_count		 = 0;
		SIZE_TYPE _item_count		 = 0;
		SIZE_TYPE _item_size_aligned = 0;
	};

	typedef pool_allocator<uint16> pool_allocator16;

}
