// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"

namespace SFG
{

	class ostream;
	class istream;

	struct render_event_model_instance
	{
		vector<uint16> materials;
		uint16		   model	   = 0;
		uint16		   mesh		   = 0;
		uint8		   single_mesh = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
