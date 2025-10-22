// Copyright (c) 2025 Inan Evin
#pragma once
#include "math_common.hpp"
#include <vendor/um/umHalf.h>

namespace SFG
{

	class vector3h
	{
	public:
		half x = 0.0f;
		half y = 0.0f;
		half z = 0.0f;

		vector3h() = default;
		vector3h(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
		{
		}

		static const vector3h zero;
		static const vector3h one;
		static const vector3h up;
		static const vector3h forward;
		static const vector3h right;
	};

}