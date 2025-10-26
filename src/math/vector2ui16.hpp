// Copyright (c) 2025 Inan Evin
#pragma once
#include "common/size_definitions.hpp"

namespace SFG
{

	class istream;
	class ostream;

	struct vector2ui16
	{
	public:
		vector2ui16() {};
		vector2ui16(uint32 _x, uint32 _y) : x(_x), y(_y) {};

		static vector2ui16 zero;
		static vector2ui16 one;

		void serialize(ostream& out) const;
		void deserialize(istream& in);

		inline bool operator==(const vector2ui16& other) const
		{
			return x == other.x && y == other.y;
		}

		uint16 x = 0;
		uint16 y = 0;
	};

}