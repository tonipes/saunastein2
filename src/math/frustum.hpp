// Copyright (c) 2025 Inan Evin
#pragma once

#include "plane.hpp"

#undef near
#undef far

namespace SFG
{
	class matrix4x4;
	class matrix3x3;
	class vector3;

	enum class frustum_result
	{
		inside,
		outside,
		intersects,
	};

	struct aabb;
	struct plane;

	struct frustum
	{
		static frustum_result test(const frustum& fr, const aabb& local_box);
		static frustum_result test(const frustum& fr, const aabb& local_box, const matrix3x3& linear_model, const vector3& position);
		static frustum_result classify_obb_vs_plane(const plane& p, const vector3& c_local, const vector3& e_local, const matrix3x3& linear_model, const vector3& position);
		static frustum		  extract(const matrix4x4& view_proj);

		plane left	 = {};
		plane right	 = {};
		plane bottom = {};
		plane top	 = {};
		plane near	 = {};
		plane far	 = {};
	};
}