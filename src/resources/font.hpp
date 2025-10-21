// Copyright (c) 2025 Inan Evin

#pragma once

#include "reflection/resource_reflection.hpp"
#include "resources/common_resources.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace vekt
{
	struct font;
}

namespace SFG
{
	struct font_raw;
	class world;

	class font
	{
	public:
		void create_from_loader(const font_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

	private:
		vekt::font* _font = nullptr;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(font, "stkfont");

}
