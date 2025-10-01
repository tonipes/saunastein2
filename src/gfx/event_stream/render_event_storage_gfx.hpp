// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_common.hpp"
#include "data/static_vector.hpp"
#include "math/vector2ui16.hpp"

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
}
