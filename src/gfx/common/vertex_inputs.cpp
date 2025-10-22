// Copyright (c) 2025 Inan Evin
#include "vertex_inputs.hpp"
#include "gfx/common/shader_description.hpp"

namespace SFG
{
	void vertex_inputs::get_vertex_inputs(vertex_input_style style, vector<vertex_input>& out_inputs)
	{
		if (style == vertex_input_style::position_normal_tangents_uv)
		{
			out_inputs.push_back({
				.name	= "POSITION",
				.offset = 0,
				.size	= sizeof(float) * 3,
				.format = format::r32g32b32_sfloat,
			});

			out_inputs.push_back({
				.name	= "NORMAL",
				.offset = sizeof(float) * 3,
				.size	= sizeof(float) * 3,
				.format = format::r32g32b32_sfloat,
			});

			out_inputs.push_back({
				.name	= "TANGENT",
				.offset = sizeof(float) * 6,
				.size	= sizeof(float) * 4,
				.format = format::r32g32b32a32_sfloat,
			});

			out_inputs.push_back({
				.name	= "TEXCOORD",
				.offset = sizeof(float) * 10,
				.size	= sizeof(float) * 2,
				.format = format::r32g32_sfloat,
			});
		}
		else if (style == vertex_input_style::position_normal_tangents_uv_skinned)
		{
			out_inputs.push_back({
				.name	= "POSITION",
				.offset = 0,
				.size	= sizeof(float) * 3,
				.format = format::r32g32b32_sfloat,
			});

			out_inputs.push_back({
				.name	= "NORMAL",
				.offset = sizeof(float) * 3,
				.size	= sizeof(float) * 3,
				.format = format::r32g32b32_sfloat,
			});

			out_inputs.push_back({
				.name	= "TANGENT",
				.offset = sizeof(float) * 6,
				.size	= sizeof(float) * 4,
				.format = format::r32g32b32a32_sfloat,
			});

			out_inputs.push_back({
				.name	= "TEXCOORD",
				.offset = sizeof(float) * 10,
				.size	= sizeof(float) * 2,
				.format = format::r32g32_sfloat,
			});

			out_inputs.push_back({
				.name	= "BLENDWEIGHT",
				.offset = sizeof(float) * 12,
				.size	= sizeof(float) * 4,
				.format = format::r32g32b32a32_sfloat,
			});

			out_inputs.push_back({
				.name	= "BLENDINDICES",
				.offset = sizeof(float) * 16,
				.size	= sizeof(uint32) * 4,
				.format = format::r32g32b32a32_uint,
			});
		}
	}
}
