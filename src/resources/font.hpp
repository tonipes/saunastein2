// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace vekt
{
	class font_manager;
	struct font;
}

namespace SFG
{
	struct font_raw;

	struct font_reflection
	{
		font_reflection();
	};

	class world;
	class chunk_allocator32;

	class font
	{
	public:
		void create_from_raw(const font_raw& raw, vekt::font_manager& fm, chunk_allocator32& alloc);
		void destroy(vekt::font_manager& fm, chunk_allocator32& alloc);

	private:
		vekt::font* _font = nullptr;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(font, resource_type::resource_type_font, font_reflection);

}
