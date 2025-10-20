// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct light_raw
	{
		string name = "";

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
