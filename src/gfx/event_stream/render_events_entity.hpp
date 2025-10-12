// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "math/matrix4x3.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct render_event_entity
	{
		matrix4x3 abs_model = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
