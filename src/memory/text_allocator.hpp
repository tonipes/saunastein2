// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"

namespace SFG
{
	class text_allocator
	{

	private:
		struct allocation
		{
			char*  ptr	= nullptr;
			size_t size = 0;
		};

	public:
		text_allocator() : _head(0){};

		void		init(uint32 capacity);
		void		uninit();
		const char* allocate(size_t len);
		const char* allocate(const char* text);
		void		deallocate(char* ptr);
		void		deallocate(const char* ptr);

		inline constexpr size_t get_capacity() const
		{
			return _capacity;
		}

		inline constexpr size_t get_head() const
		{
			return _head;
		}

		inline char* get_raw() const
		{
			return _raw;
		}

		inline void reset()
		{
			_free_list.resize(0);
			_head = 0;
		}

	private:
		vector<allocation> _free_list;
		char*			   _raw		 = nullptr;
		uint32			   _head	 = 0;
		uint32			   _capacity = 0;
	};

}
