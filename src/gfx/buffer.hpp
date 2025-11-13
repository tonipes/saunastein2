// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "data/bitmask.hpp"

namespace SFG
{
	struct resource_desc;

	class buffer
	{
	public:
		void create_staging_hw(const resource_desc& staging, const resource_desc& hw);
		void create_hw(const resource_desc& desc);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_id cmd_buffer);
		void copy_region(gfx_id cmd_buffer, size_t padding, size_t size);

		inline gfx_id get_hw_staging() const
		{
			return _hw_staging;
		}
		inline gfx_id get_hw_gpu() const
		{
			return _hw_gpu;
		}

		inline bool is_dirty() const
		{
			return _flags.is_set(buffer_flags::buf_dirty);
		}

		inline bool is_alive() const
		{
			return _flags.is_set(buffer_flags::buf_alive);
		}

		inline uint32 get_gpu_index() const
		{
			return _gpu_heap_index;
		}

	private:
		enum buffer_flags
		{
			buf_has_staging = 1 << 0,
			buf_dirty		= 1 << 2,
			buf_alive		= 1 << 3,
		};

	private:
		uint8*		   _mapped		   = nullptr;
		uint32		   _total_size	   = 0;
		uint32		   _gpu_heap_index = 0;
		gfx_id		   _hw_staging	   = NULL_GFX_ID;
		gfx_id		   _hw_gpu		   = NULL_GFX_ID;
		bitmask<uint8> _flags		   = 0;
	};

	class simple_buffer_cpu_gpu
	{
	public:
		void create(const resource_desc& staging, const resource_desc& hw);
		void destroy();

		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_id cmd_buffer);
		void copy_region(gfx_id cmd_buffer, size_t padding, size_t size);

		inline gfx_id get_hw_staging() const
		{
			return _hw_staging;
		}
		inline gfx_id get_hw_gpu() const
		{
			return _hw_gpu;
		}

		inline uint8* get_mapped() const
		{
			return _mapped;
		}

	private:
		uint8* _mapped	   = nullptr;
		gfx_id _hw_staging = NULL_GFX_ID;
		gfx_id _hw_gpu	   = NULL_GFX_ID;
	};

}
