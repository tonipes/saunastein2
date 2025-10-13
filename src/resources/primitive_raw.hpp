// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "vertex.hpp"
#include "math/aabb.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct primitive_static_raw
	{
		aabb   local_aabb	  = {};
		uint16 material_index = 0;

		vector<vertex_static>	vertices;
		vector<primitive_index> indices;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct primitive_skinned_raw
	{
		aabb					local_aabb	   = {};
		uint16					material_index = 0;
		vector<vertex_skinned>	vertices;
		vector<primitive_index> indices;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
