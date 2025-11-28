// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "math/matrix4x3.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct model_node_raw;
	class world;

	class model_node
	{
	public:
		void create_from_loader(const model_node_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

#ifndef SFG_STRIP_DEBUG_NAMES
		inline chunk_handle32 get_name() const
		{
			return _name;
		}
#endif

		inline int16 get_parent_index() const
		{
			return _parent_index;
		}

		inline int16 get_mesh_index() const
		{
			return _mesh_index;
		}

		inline int16 get_skin_index() const
		{
			return _skin_index;
		}

		inline int16 get_light_index() const
		{
			return _light_index;
		}

		inline const matrix4x3& get_local_matrix() const
		{
			return _local_matrix;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		int16	  _parent_index = -1;
		int16	  _light_index	= -1;
		int16	  _mesh_index	= -1;
		int16	  _skin_index	= -1;
		matrix4x3 _local_matrix = {};
	};
}