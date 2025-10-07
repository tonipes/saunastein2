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
#ifndef SFG_STRIP_DEBUG_NAMES
		const char* name = "";
#endif
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		vector2ui16										size			  = {};
		uint32											intermediate_size = 0;
		uint8											format			  = 0;
	};

	struct render_event_storage_sampler
	{
		sampler_desc desc = {};
	};

	struct render_event_storage_shader
	{
		shader_desc desc = {};
	};

	struct render_event_storage_material
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		const char* name = "";
#endif
		ostream												  data = {};
		static_vector<resource_handle, MAX_MATERIAL_TEXTURES> textures;
	};

	struct render_event_storage_mesh
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		const char* name = "";
#endif
		vector<primitive_static_raw>  primitives_static;
		vector<primitive_skinned_raw> primitives_skinned;
	};
}
