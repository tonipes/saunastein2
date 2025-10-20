// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"
#include "gfx/common/descriptions.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct texture_sampler_raw;

	struct texture_sampler_reflection
	{
		texture_sampler_reflection();
	};

	class render_event_stream;
	class chunk_allocator32;

	class texture_sampler
	{
	public:
		void create_from_raw(const texture_sampler_raw& raw, render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);
		void destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);

		inline const sampler_desc& get_desc() const
		{
			return _desc;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif

		sampler_desc _desc = {};
	};

	REGISTER_RESOURCE(texture_sampler, resource_type::resource_type_texture_sampler, texture_sampler_reflection);

}
