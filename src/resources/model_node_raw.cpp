// Copyright (c) 2025 Inan Evin

#include "model_node_raw.hpp"
#include "model_node.hpp"
#include "io/log.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void model_node_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << parent_index;
		stream << mesh_index;
		stream << skin_index;
		stream << local_matrix;
		stream << light_index;
	}

	void model_node_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> parent_index;
		stream >> mesh_index;
		stream >> skin_index;
		stream >> local_matrix;
		stream >> light_index;
		SFG_INFO("Created model node from buffer: {0}", name);
	}
}
