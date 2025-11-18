// Copyright (c) 2025 Inan Evin

#include "vector4ui16.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	vector4ui16 vector4ui16::zero = vector4ui16(0, 0, 0, 0);
	vector4ui16 vector4ui16::one  = vector4ui16(1, 1, 1, 1);

	void vector4ui16::serialize(ostream& stream) const
	{
		stream << x << y << z << w;
	}

	void vector4ui16::deserialize(istream& stream)
	{
		stream >> x >> y >> z >> w;
	}

}