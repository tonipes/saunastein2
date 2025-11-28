// Copyright (c) 2025 Inan Evin

#include "vector4i.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"

namespace SFG
{
	vector4i vector4i::zero = vector4i(0, 0, 0, 0);
	vector4i vector4i::one	= vector4i(1, 1, 1, 1);

	void vector4i::serialize(ostream& stream) const
	{
		stream << x << y << z << w;
	}

	void vector4i::deserialize(istream& stream)
	{
		stream >> x >> y >> z >> w;
	}

}