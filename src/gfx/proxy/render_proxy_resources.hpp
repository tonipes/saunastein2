// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/gfx_common.hpp"
#include "gfx/buffer.hpp"
#include "data/ostream.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	enum render_proxy_status : uint8
	{
		rps_inactive = 0,
		rps_active	 = 1,
		rps_obsolete,
	};

	struct render_proxy_texture
	{
		uint16 handle		= {};
		gfx_id hw			= 0;
		gfx_id intermediate = 0;
		uint8  status		= render_proxy_status::rps_inactive;
	};

	struct render_proxy_material
	{
		buffer												buffers[FRAMES_IN_FLIGHT];
		static_vector<uint16, MAX_MATERIAL_SHADER_VARIANTS> shader_handles;
		static_vector<uint16, MAX_MATERIAL_TEXTURES>		texture_handles;
		uint16												handle = {};
		gfx_id												bind_groups[FRAMES_IN_FLIGHT];
		uint8												status = render_proxy_status::rps_inactive;
	};

	struct render_proxy_shader
	{
		uint16 handle = {};
		gfx_id hw	  = 0;
		uint8  status = render_proxy_status::rps_inactive;
	};

	struct render_proxy_sampler
	{
		uint16 handle = {};
		gfx_id hw	  = 0;
		uint8  status = render_proxy_status::rps_inactive;
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
		buffer		   vertex_buffer = {};
		buffer		   index_buffer	 = {};
		chunk_handle32 primitives;
		uint32		   primitive_count = 0;
		uint16		   handle		   = {};
		uint16		   material_count  = 0;
		uint8		   status		   = render_proxy_status::rps_inactive;
	};

}
