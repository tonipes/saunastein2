// Copyright (c) 2025 Inan Evin
#pragma once

#include "gfx/world/renderable.hpp"
#include "data/vector.hpp"
#include "world/world_max_defines.hpp"

namespace SFG
{
	struct view;
	class draw_stream;
	class proxy_manager;

	class renderable_collector
	{
	public:
		renderable_collector() = delete;

		static void collect_model_instances(proxy_manager& pm, const view& view, vector<renderable_object>& out_objects);
		static void populate_draw_stream(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream& stream, uint32 skip_material_flags, uint32 base_variant_flags, uint8 frame_index);
	};
}
