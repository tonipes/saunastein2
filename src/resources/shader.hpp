// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/bitmask.hpp"
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

	class render_event_stream;

	class shader
	{
	public:
		enum flags
		{
			is_skinned = 1 << 0,
		};

		void create_from_raw(const shader_raw& raw, render_event_stream& stream, resource_handle handle);
		void destroy(render_event_stream& stream, resource_handle handle);

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

	private:
		bitmask<uint8> _flags = 0;
	};

	REGISTER_TYPE(shader, resource_type::resource_type_shader);

}
