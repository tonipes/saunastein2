// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/vector.hpp"
#include "data/string.hpp"
#include "math/vector2ui16.hpp"

namespace vekt
{
	class builder;
	struct font;
}

namespace SFG
{
	class world;

	// Lightweight description of current editor panels/widgets.
	// Holds content state (e.g., entity name list) and knows how to build
	// vekt widgets onto a provided builder each frame.
	class editor_panels
	{
	public:
		void init();
		void uninit();

		// Refresh backing data from world (entity names etc.).
		void update_from_world(world& w);

		// Build widgets into the given vekt builder for the current screen size.
		void build_ui(vekt::builder& b, const vector2ui16& screen_size, vekt::font* default_font);

		inline const SFG::vector<SFG::string>& get_entity_names() const
		{
			return _entity_names;
		}

	private:
		SFG::vector<SFG::string> _entity_names;
	};
}
