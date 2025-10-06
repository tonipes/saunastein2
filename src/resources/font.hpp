// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"

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

	extern font_reflection g_font_reflection;

	class world;
	class chunk_allocator32;

	class font
	{
	public:
		void create_from_raw(const font_raw& raw, vekt::font_manager& fm, chunk_allocator32& alloc);
		void destroy(vekt::font_manager& fm, chunk_allocator32& alloc);

	private:
		vekt::font*	   _font = nullptr;
		chunk_handle32 _name = {};
	};

	REGISTER_TYPE(font, resource_type::resource_type_font);

}
