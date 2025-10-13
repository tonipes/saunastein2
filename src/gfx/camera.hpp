// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct vector2ui16;
	class vector3;
	class quat;

	class camera
	{
	public:
		static matrix4x4 view(const quat& rot, const vector3& pos);
		static matrix4x4 proj(float fov_degrees, const vector2ui16& size, float near, float far);
		static matrix4x4 ortho(const vector2ui16& size, float near, float far);
	};
}
