// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "common/string_id.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct skin_joint
	{
		uint16	  model_node_index	  = 0;
		matrix4x4 inverse_bind_matrix = {};
		string_id name_hash			  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}