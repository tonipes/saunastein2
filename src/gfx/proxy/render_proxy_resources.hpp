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

#include "gfx/buffer.hpp"
#include "math/aabb.hpp"
#include "data/ostream.hpp"
#include "data/bitmask.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_handle.hpp"
#include "render_proxy_common.hpp"
#include "resources/common_resources.hpp"
#include "game/game_max_defines.hpp"
#include "world/particles/common_particles.hpp"

namespace SFG
{
	struct render_proxy_custom_buffer
	{
		buffer_cpu_gpu buffers[BACK_BUFFER_COUNT] = {};
		uint32		   size						  = 0;
		resource_id	   handle					  = {};
		uint8		   status					  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_texture
	{
		resource_id handle		 = {};
		gpu_index	heap_index	 = 0;
		gfx_id		hw			 = 0;
		gfx_id		intermediate = 0;
		uint8		status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_material
	{
		buffer_gpu buffers[BACK_BUFFER_COUNT];
		buffer_gpu texture_buffers[BACK_BUFFER_COUNT] = {};
		uint32	   buffer_size						  = 0;
		uint8	   texture_count					  = 0;
		uint8	   status							  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_material_runtime
	{
		gpu_index		gpu_index_buffers[BACK_BUFFER_COUNT]		 = {NULL_GPU_INDEX};
		gpu_index		gpu_index_texture_buffers[BACK_BUFFER_COUNT] = {NULL_GPU_INDEX};
		gpu_index		gpu_index_sampler							 = NULL_GPU_INDEX;
		bitmask<uint32> flags										 = 0;
		resource_id		shader_handle								 = NULL_RESOURCE_ID;
		uint16			draw_priority								 = 0;
	};

	struct render_proxy_shader_variant
	{
		gfx_id			hw = 0;
		bitmask<uint32> variant_flags;
	};

	struct render_proxy_shader
	{
		resource_id	   handle		 = {};
		chunk_handle32 variants		 = {};
		uint32		   variant_count = 0;
		uint8		   status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_sampler
	{
		resource_id handle	   = {};
		gpu_index	heap_index = 0;
		gfx_id		hw		   = 0;
		uint8		status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_primitive
	{
		uint32 vertex_start	  = 0;
		uint32 index_start	  = 0;
		uint32 index_count	  = 0;
		uint16 material_index = 0;
	};

	struct render_proxy_mesh
	{
		buffer_cpu_gpu vertex_buffer = {};
		buffer_cpu_gpu index_buffer	 = {};
		aabb		   local_aabb	 = {};
		chunk_handle32 primitives;
		uint32		   primitive_count = 0;
		resource_id	   handle		   = {};
		uint8		   status		   = render_proxy_status::rps_inactive;
		uint8		   is_skinned	   = 0;
	};

	struct render_proxy_skin
	{
		chunk_handle32 nodes	  = {};
		chunk_handle32 matrices	  = {};
		uint16		   node_count = 0;
		int16		   root_node  = -1;
		uint8		   status	  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_particle_resource
	{
		particle_emit_properties emit_props = {};
		uint8					 status		= render_proxy_status::rps_inactive;
	};

}
