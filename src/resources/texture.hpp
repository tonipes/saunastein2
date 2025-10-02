// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/bitmask.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "common/type_id.hpp"
#include "common_resources.hpp"

namespace SFG
{
	struct texture_raw;

	struct texture_reflection
	{
		texture_reflection();
	};

	extern texture_reflection g_texture_reflection;

	class render_event_stream;
	class world_resources;

	class texture
	{
	public:
		~texture();

		void create_from_raw(const texture_raw& raw, render_event_stream& stream, resource_handle handle);
		void destroy(render_event_stream& stream, resource_handle handle);

	private:
		uint8 _texture_format = 0;
	};

	REGISTER_TYPE(texture, resource_type::resource_type_texture);
}