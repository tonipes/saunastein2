// Copyright (c) 2025 Inan Evin
#pragma once

#include "math_common.hpp"
#include <vendor/um/umHalf.h>

#undef min
#undef max

namespace SFG
{
	class istream;
	class ostream;

	class vector2h
	{
	public:
		vector2h() {};
		vector2h(float _x, float _y) : x(_x), y(_y) {};

		half x = 0.0f;
		half y = 0.0f;

		static vector2h zero;
		static vector2h one;
	};

}