// Copyright (c) 2025 Inan Evin
#pragma once
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct mesh_raw;
	class world;

	class mesh
	{
	public:
		void create_from_loader(const mesh_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		inline uint16 get_node_index() const
		{
			return _node_index;
		}

		inline uint16 get_skin_index() const
		{
			return _skin_index;
		}

		inline string_id get_sid() const
		{
			return _sid;
		}

	private:
		friend class model;

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		uint16	  _node_index = 0;
		int16	  _skin_index = 0;
		string_id _sid		  = 0;
	};

	REGISTER_RESOURCE(mesh, "");

}