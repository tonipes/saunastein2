// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/gfx_common.hpp"
#include "gfx/buffer.hpp"
#include "data/ostream.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	struct render_proxy_texture
	{
		uint16 handle		= {};
		gfx_id hw			= 0;
		gfx_id intermediate = 0;
		uint8  active		= 0;
	};

	struct render_proxy_material
	{
		uint16	handle = {};
		uint8	active = 0;
		gfx_id	bind_groups[FRAMES_IN_FLIGHT];
		buffer	buffers[FRAMES_IN_FLIGHT];
		ostream data = {};
	};

	struct render_proxy_shader
	{
		uint16 handle = {};
		gfx_id hw	  = 0;
		uint8  active = 0;
	};

	struct render_proxy_sampler
	{
		uint16 handle = {};
		gfx_id hw	  = 0;
		uint8  active = 0;
	};

	struct render_proxy_primitive
	{
		uint16 material_index = 0;
		uint32 vertex_start	  = 0;
		uint32 index_start	  = 0;
		uint32 index_count	  = 0;
	};

	struct render_proxy_mesh
	{
		uint16		   handle = {};
		uint8		   active = 0;
		chunk_handle32 primitives;
		uint32		   primitive_count = 0;
		uint16		   material_count  = 0;
		buffer		   vertex_buffer   = {};
		buffer		   index_buffer	   = {};
	};

}
