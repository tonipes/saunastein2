// Copyright (c) 2025 Inan Evin

#include "camera.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"

namespace SFG
{
	matrix4x4 camera::view(const quat& rot, const vector3& pos)
	{
		const matrix4x4 rot_mat		= matrix4x4::rotation(rot.inverse());
		const matrix4x4 translation = matrix4x4::translation(-pos);
		return rot_mat * translation;
	}

	matrix4x4 camera::proj(float fov_degrees, const vector2ui16& size, float near, float far)
	{
		return matrix4x4::perspective(fov_degrees, static_cast<float>(size.x) / static_cast<float>(size.y), near, far);
	}

	matrix4x4 camera::ortho(const vector2ui16& size, float near, float far)
	{
		return matrix4x4::ortho(-0.5f * size.x, 0.5f * size.x, -0.5f * size.y, 0.5f * size.y, near, far);
	}
} // namespace Lina
