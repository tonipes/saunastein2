// Copyright (c) 2025 Inan Evin
#pragma once
#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"
#include "common/type_id.hpp"

namespace SFG
{
	class chunk_allocator32;

	struct mesh_raw;

	struct mesh_reflection
	{
		mesh_reflection();
	};
	extern mesh_reflection g_mesh_reflection;

	class render_event_stream;

	class mesh
	{
	public:
		void create_from_raw(const mesh_raw& raw, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);
		void destroy(chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);

		inline chunk_handle32 get_material_indices() const
		{
			return _material_indices;
		}

		inline uint16 get_node_index() const
		{
			return _node_index;
		}

		inline uint16 get_material_count() const
		{
			return _material_count;
		}

	private:
		friend class model;

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _material_indices; // original indices into the loaded model.
		uint16		   _node_index	   = 0;
		int16		   _skin_index	   = 0;
		uint16		   _material_count = 0;
	};
	REGISTER_TYPE(mesh, resource_type::resource_type_mesh);

}