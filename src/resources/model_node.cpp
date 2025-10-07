// Copyright (c) 2025 Inan Evin

#include "model_node.hpp"
#include "model_node_raw.hpp"
#include "memory/chunk_allocator.hpp"

namespace SFG
{
	void model_node::destroy(chunk_allocator32& alloc)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif
	}

	void model_node::create_from_raw(const model_node_raw& raw, chunk_allocator32& alloc)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif
		_parent_index = raw.parent_index;
		_mesh_index	  = raw.mesh_index;
		_local_matrix = raw.local_matrix;
	}

}