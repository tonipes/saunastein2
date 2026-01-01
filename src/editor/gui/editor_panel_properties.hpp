#pragma once

#include "gui/vekt_defines.hpp"
#include "world/world_constants.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class world;

	enum class editor_property_type : uint8
	{
		entity = 0,
	};

	class editor_panel_properties
	{
	public:
		void init(vekt::builder* builder);
		void uninit();
		void draw(world& w, world_handle selected, const struct vector2ui16& window_size);

	private:
		vekt::builder* _builder = nullptr;
		vekt::id _w_window = NULL_WIDGET_ID;
		editor_property_type _type = editor_property_type::entity;
	};
}

