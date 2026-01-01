#pragma once

#include "gui/vekt_defines.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class world;

	class editor_panel_entities
	{
	public:
		void init(vekt::builder* builder);
		void uninit();
		void draw(world& w, const struct vector2ui16& window_size);

	private:
		vekt::builder* _builder = nullptr;
		vekt::id _w_window = NULL_WIDGET_ID;
	};
}

