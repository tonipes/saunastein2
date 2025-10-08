// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct texture_raw;

	struct texture_reflection
	{
		texture_reflection();
	};

	class render_event_stream;
	class world_resources;
	class chunk_allocator32;

	class texture
	{
	public:
		~texture();

		void create_from_raw(const texture_raw& raw, render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);
		void destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);

	private:
		uint8 _texture_format = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(texture, resource_type::resource_type_texture, texture_reflection);
}