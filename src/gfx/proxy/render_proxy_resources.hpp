// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/gfx_common.hpp"
#include "gfx/buffer.hpp"
#include "math/aabb.hpp"
#include "data/ostream.hpp"
#include "data/bitmask.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_handle.hpp"
#include "render_proxy_common.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{

	struct render_proxy_texture
	{
		resource_id handle		 = {};
		gfx_id		hw			 = 0;
		gfx_id		intermediate = 0;
		uint8		status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_material
	{
		buffer													 buffers[BACK_BUFFER_COUNT];
		static_vector<resource_id, MAX_MATERIAL_SHADER_VARIANTS> shader_handles;
		static_vector<resource_id, MAX_MATERIAL_TEXTURES>		 texture_handles;
		resource_id												 handle = {};
		gfx_id													 bind_groups[BACK_BUFFER_COUNT];
		uint8													 status = render_proxy_status::rps_inactive;
		bitmask<uint8>											 flags	= 0;
	};

	struct render_proxy_shader
	{
		resource_id	   handle = {};
		gfx_id		   hw	  = 0;
		uint8		   status = render_proxy_status::rps_inactive;
		bitmask<uint8> flags  = 0;
	};

	struct render_proxy_sampler
	{
		resource_id handle = {};
		gfx_id		hw	   = 0;
		uint8		status = render_proxy_status::rps_inactive;
	};

	struct render_proxy_primitive
	{
		uint32 vertex_start	  = 0;
		uint32 index_start	  = 0;
		uint32 index_count	  = 0;
		uint16 material_index = 0;
	};

	struct render_proxy_model
	{
		chunk_handle32 meshes;
		chunk_handle32 materials;
		uint32		   mesh_count	  = 0;
		uint32		   material_count = 0;
		uint8		   status		  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_mesh
	{
		buffer		   vertex_buffer = {};
		buffer		   index_buffer	 = {};
		aabb		   local_aabb	 = {};
		chunk_handle32 primitives;
		uint32		   primitive_count = 0;
		resource_id	   handle		   = {};
		uint8		   status		   = render_proxy_status::rps_inactive;
		uint8		   is_skinned	   = 0;
	};

}
