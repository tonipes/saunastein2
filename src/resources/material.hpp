// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/bitmask.hpp"
#include "data/ostream.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	class world_resources;
	struct material_raw;

	struct material_reflection
	{
		material_reflection();
	};

	class render_event_stream;
	class chunk_allocator32;

	class material
	{
	public:
		void create_from_raw(const material_raw& raw, world_resources& resources, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);
		void destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);
		void update_data(render_event_stream& stream, resource_handle handle);

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

		inline ostream& get_data()
		{
			return _material_data;
		}

	private:
		ostream		   _material_data = {};
		bitmask<uint8> _flags		  = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(material, resource_type::resource_type_material, material_reflection);

}
