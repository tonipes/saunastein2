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

#include "buffer.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "memory/memory.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"

namespace SFG
{

	void buffer::create_staging_hw(const resource_desc& staging, const resource_desc& hw)
	{
		gfx_backend* backend = gfx_backend::get();
		_total_size			 = staging.size;
		_hw_staging			 = backend->create_resource(staging);
		_hw_gpu				 = backend->create_resource(hw);
		backend->map_resource(_hw_staging, _mapped);
		_flags.set(buf_has_staging | buf_alive);

		_gpu_heap_index = backend->get_resource_gpu_index(_hw_gpu);
	}

	void buffer::create_hw(const resource_desc& desc)
	{
		gfx_backend* backend = gfx_backend::get();
		_total_size			 = desc.size;
		_hw_gpu				 = backend->create_resource(desc);
		backend->map_resource(_hw_gpu, _mapped);
		_flags.set(buf_alive);

		_gpu_heap_index = backend->get_resource_gpu_index(_hw_gpu);
	}

	void buffer::destroy()
	{
		gfx_backend* backend = gfx_backend::get();
		if (_flags.is_set(buffer_flags::buf_has_staging))
			backend->destroy_resource(_hw_staging);

		backend->destroy_resource(_hw_gpu);
		_mapped = nullptr;
		_flags.remove(buffer_flags::buf_alive);
		_flags.remove(buffer_flags::buf_has_staging);
		_gpu_heap_index = UINT32_MAX;
		_hw_gpu			= NULL_GFX_ID;
	}

	void buffer::buffer_data(size_t padding, const void* data, size_t size)
	{
		SFG_ASSERT(padding + size <= _total_size);
		SFG_ASSERT(size != 0);
		SFG_MEMCPY(_mapped + padding, data, size);
		_flags.set(buf_dirty);
	}

	void buffer::copy(gfx_id cmd_buffer)
	{
		SFG_ASSERT(_flags.is_set(buffer_flags::buf_has_staging));
		SFG_ASSERT(_flags.is_set(buffer_flags::buf_dirty));

		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource(cmd_buffer,
								   {
									   .source		= _hw_staging,
									   .destination = _hw_gpu,
								   });
		_flags.remove(buf_dirty);
	}

	void buffer::copy_region(gfx_id cmd_buffer, size_t padding, size_t size)
	{
		SFG_ASSERT(_flags.is_set(buffer_flags::buf_has_staging));
		SFG_ASSERT(_flags.is_set(buffer_flags::buf_dirty));
		SFG_ASSERT(size != 0);

		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource_region(cmd_buffer,
										  {
											  .source	   = _hw_staging,
											  .destination = _hw_gpu,
											  .dst_offset  = padding,
											  .src_offset  = padding,
											  .size		   = size,
										  });
		_flags.remove(buf_dirty);
	}

	void simple_buffer_cpu_gpu::create(const resource_desc& staging, const resource_desc& hw)
	{
		gfx_backend* backend = gfx_backend::get();
		_hw_staging			 = backend->create_resource(staging);
		_hw_gpu				 = backend->create_resource(hw);
		backend->map_resource(_hw_staging, _mapped);
	}

	void simple_buffer_cpu_gpu::destroy()
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_resource(_hw_staging);
		backend->destroy_resource(_hw_gpu);
		_mapped		= nullptr;
		_hw_staging = NULL_GFX_ID;
		_hw_gpu		= NULL_GFX_ID;
	}

	void simple_buffer_cpu_gpu::buffer_data(size_t padding, const void* data, size_t size)
	{
		SFG_MEMCPY(_mapped + padding, data, size);
	}

	void simple_buffer_cpu_gpu::copy(gfx_id cmd_buffer)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource(cmd_buffer,
								   {
									   .source		= _hw_staging,
									   .destination = _hw_gpu,
								   });
	}

	void simple_buffer_cpu_gpu::copy_region(gfx_id cmd_buffer, size_t padding, size_t size)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource_region(cmd_buffer,
										  {
											  .source	   = _hw_staging,
											  .destination = _hw_gpu,
											  .dst_offset  = padding,
											  .src_offset  = padding,
											  .size		   = size,
										  });
	}

}
