// Copyright (c) 2025 Inan Evin
#pragma once

namespace vekt
{
	class builder;
	struct font;
}

namespace SFG
{

	struct vector2ui16;

	class editor_gui_world_overlays
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* builder);
		void draw(vekt::builder* builder, const vector2ui16& size);

	private:
	};
}
