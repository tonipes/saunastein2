// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_common.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/static_vector.hpp"
#include "math/vector2ui16.hpp"
#include "resources/primitive_raw.hpp"
#include "data/ostream.hpp"

namespace SFG
{
	struct render_event_storage_texture
	{
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		const char*										name			  = "";
		vector2ui16										size			  = {};
		uint32											intermediate_size = 0;
		uint8											format			  = 0;
	};

	struct render_event_storage_sampler
	{
		sampler_desc desc = {};
		const char*	 name = "";
	};

	struct render_event_storage_shader
	{
		shader_desc desc = {};
		const char* name = "";
	};

	struct render_event_storage_material
	{
		ostream												  data = {};
		const char*											  name = "";
		static_vector<resource_handle, MAX_MATERIAL_TEXTURES> textures;
	};

	struct render_event_storage_mesh
	{
		vector<primitive_static_raw>  primitives_static;
		vector<primitive_skinned_raw> primitives_skinned;
	};
}
