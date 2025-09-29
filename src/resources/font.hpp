// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"

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

	class font
	{
	public:
		void create_from_raw(const font_raw& raw, vekt::font_manager& fm);
		void destroy(vekt::font_manager& fm);

	private:
		vekt::font* _font = nullptr;
	};

	REGISTER_TYPE(font, resource_type::resource_type_font);

}
