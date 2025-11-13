// Copyright (c) 2025 Inan Evin

#pragma once

#include "physics/common_physics.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/color.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Physics/Body/MotionType.h>

namespace SFG
{
	static inline JPH::Vec3 to_jph_vec3(const vector3& v)
	{
		return JPH::Vec3(v.x, v.y, v.z);
	}

	static inline JPH::Quat to_jph_quat(const quat& q)
	{
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	static inline JPH::Color to_jph_color(const color& c)
	{
		return JPH::Color(c.x, c.y, c.z, c.w);
	}

	static inline vector3 from_jph_vec3(const JPH::Vec3& v)
	{
		return vector3(v.GetX(), v.GetY(), v.GetZ());
	}

	static inline quat from_jph_quat(const JPH::Quat& q)
	{
		return quat(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
	}

	static inline color from_jph_color(const JPH::Color& c)
	{
		return color(c.r, c.g, c.b, c.a);
	}
}
