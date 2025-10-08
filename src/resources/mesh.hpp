// Copyright (c) 2025 Inan Evin
#pragma once
#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	class chunk_allocator32;

	struct mesh_raw;

	struct mesh_reflection
	{
		mesh_reflection();
	};

	class render_event_stream;

	class mesh
	{
	public:
		void create_from_raw(const mesh_raw& raw, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);
		void destroy(chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);

		inline uint16 get_node_index() const
		{
			return _node_index;
		}

		inline uint16 get_skin_index() const
		{
			return _skin_index;
		}

	private:
		friend class model;

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		uint16 _node_index = 0;
		int16  _skin_index = 0;
	};

	REGISTER_RESOURCE(mesh, resource_type::resource_type_mesh, mesh_reflection);

}