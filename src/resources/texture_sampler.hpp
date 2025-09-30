// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"
#include <limits>

namespace SFG
{
	struct texture_sampler_raw;

	struct texture_sampler_reflection
	{
		texture_sampler_reflection();
	};

	extern texture_sampler_reflection g_texture_sampler_reflection;

	class render_event_stream;

	class texture_sampler
	{
	public:
		~texture_sampler();

		void create_from_raw(const texture_sampler_raw& raw);
		void push_render_event(render_event_stream& stream, resource_handle handle);
		void destroy();

		inline gfx_id get_hw() const
		{
			return _hw;
		}

	private:
		gfx_id _hw = std::numeric_limits<gfx_id>::max();
	};

	REGISTER_TYPE(texture_sampler, resource_type::resource_type_texture_sampler);

}
