// Copyright (c) 2025 Inan Evin

#include "light_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{

	void light_raw::serialize(ostream& stream) const
	{
		stream << base_color;
		stream << intensity;
		stream << range;
		stream << inner_cone;
		stream << outer_cone;
		stream << type;
	}
	void light_raw::deserialize(istream& stream)
	{
		stream >> base_color;
		stream >> intensity;
		stream >> range;
		stream >> inner_cone;
		stream >> outer_cone;
		stream >> type;
	}

}
