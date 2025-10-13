// Copyright (c) 2025 Inan Evin

#include "mesh_raw.hpp"
#include "mesh.hpp"
#include "primitive.hpp"
#include "io/log.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{

	void mesh_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << sid;
		stream << node_index;
		stream << primitives_static;
		stream << primitives_skinned;
		stream << skin_index;
		stream << local_aabb;
	}

	void mesh_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> sid;
		stream >> node_index;
		stream >> primitives_static;
		stream >> primitives_skinned;
		stream >> skin_index;
		stream >> local_aabb;

		SFG_INFO("Created mesh from buffer: {0}", name);
	}

}
