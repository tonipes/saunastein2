// Copyright (c) 2025 Inan Evin

#include "model_node.hpp"
#include "model_node_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"

namespace SFG
{

	void model_node::create_from_loader(const model_node_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif
		_parent_index = raw.parent_index;
		_mesh_index	  = raw.mesh_index;
		_local_matrix = raw.local_matrix;
		_light_index  = raw.light_index;
		_skin_index	  = raw.skin_index;
	}

	void model_node::destroy(world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif
	}

}