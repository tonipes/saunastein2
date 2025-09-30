// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_event_common.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_common.hpp"
#include "resources/common_resources.hpp"
#include "data/static_vector.hpp"
#include <functional>

namespace SFG
{

	struct texture_data_storage
	{
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		gfx_id											intermediate_buffer = 0;
		gfx_id											hw					= 0;
	};

	struct render_event
	{
		std::function<void(void* data_storage)> create_callback;
		std::function<void(void* data_storage)> destroy_callback;
		resource_handle							handle;
		render_event_type						event_type;
	};
}
