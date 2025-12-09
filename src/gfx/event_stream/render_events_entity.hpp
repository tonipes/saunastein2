// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "math/matrix4x3.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct render_event_entity_transform
	{
		matrix4x3 abs_model = matrix4x3::identity;
		vector3	  position	= vector3::zero;
		quat	  rotation	= quat::identity;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_entity_visibility
	{
		uint8 visible = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

}
