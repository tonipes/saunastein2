// Copyright (c) 2025 Inan Evin

#include "editor_gui_world_overlays.hpp"
#include "editor_gui_theme.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector2.hpp"

// vekt
#include "gui/vekt.hpp"

namespace SFG
{
	void editor_gui_world_overlays::init(vekt::builder* builder)
	{
	}

	void editor_gui_world_overlays::draw(vekt::builder* builder, const vector2ui16& size)
	{
		const vector2 box_size = vector2(static_cast<float>(size.x) * 0.01f, static_cast<float>(size.x) * 0.01f);
		const vector2 min	   = vector2(editor_gui_theme::default_margin, size.y - editor_gui_theme::default_margin - box_size.y);
		const vector2 max	   = min + box_size;
		const vector2 center   = (min + max) * 0.5f;


	}
}