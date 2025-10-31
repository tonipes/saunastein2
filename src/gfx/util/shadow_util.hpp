// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/static_vector.hpp"
#include "math/vector4.hpp"

namespace SFG
{
	struct vector2ui16;
	struct vector2;
	class matrix4x4;
	class vector3;

	class shadow_util
	{
	public:
		static void get_world_space_ndc(const matrix4x4& inv_view_proj, static_vector<vector4, 8>& out_world_space, vector3& out_center);
		static void get_lightspace_projection(matrix4x4& out_proj, const matrix4x4& light_view, const static_vector<vector4, 8>& world_space_ndc, const vector2ui16& resolution, vector2& out_texel_size);
	};
}