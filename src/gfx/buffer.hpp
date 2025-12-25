/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

		inline void set_dirty(bool d)
		{
			_flags.set(buffer_flags::buf_dirty, d);
		}

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

		inline uint8* get_mapped() const
		{
			return _mapped;
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
