// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/vector.hpp"

namespace SFG
{
	struct vertex_input;

	enum class vertex_input_style
	{
		position_normal_tangents_uv,
		position_normal_tangents_uv_skinned,
		line_3d,
		position_color,
		gui
	};

	class vertex_inputs
	{
	public:
		static void get_vertex_inputs(vertex_input_style, vector<vertex_input>& out_inputs);
	};
}