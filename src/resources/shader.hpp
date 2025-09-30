// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"
#include "data/bitmask.hpp"
#include "gfx/common/shader_description.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct shader_raw;

	struct shader_reflection
	{
		shader_reflection();
	};

	extern shader_reflection g_shader_reflection;

	class world;
	class render_event_stream;

	class shader
	{
	public:
		enum flags
		{
			is_skinned = 1 << 0,
			hw_exists  = 1 << 1,
		};

		~shader();

		void   create_from_raw(const shader_raw& raw, bool use_embedded_layout, gfx_id layout);
		void   push_create_event(render_event_stream& stream, resource_handle handle);
		void   destroy();
		gfx_id get_hw() const;

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

	private:
		gfx_id		   _hw	  = 0;
		bitmask<uint8> _flags = 0;
	};

	REGISTER_TYPE(shader, resource_type::resource_type_shader);

}
