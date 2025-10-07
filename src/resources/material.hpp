// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "data/bitmask.hpp"
#include "data/ostream.hpp"
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	class world_resources;
	struct material_raw;

	struct material_reflection
	{
		material_reflection();
	};

	extern material_reflection g_material_reflection;

	class render_event_stream;
	class chunk_allocator32;

	class material
	{
	public:
		enum flags
		{
			is_gbuffer = 1 << 0,
			is_forward = 1 << 1,
		};

		void			create_from_raw(const material_raw& raw, world_resources& resources, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle);
		void			destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle);
		resource_handle get_shader(uint8 flags_to_match) const;
		void			update_data(render_event_stream& stream, resource_handle handle);

		inline resource_handle get_shader() const
		{
			SFG_ASSERT(!_all_shaders.empty());
			return _all_shaders[0];
		}

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

		inline ostream& get_data()
		{
			return _material_data;
		}

	private:
		ostream														 _material_data = {};
		static_vector<resource_handle, MAX_MATERIAL_SHADER_VARIANTS> _all_shaders;
		static_vector<bitmask<uint8>, MAX_MATERIAL_SHADER_VARIANTS>	 _all_shader_flags;
		bitmask<uint8>												 _flags = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_TYPE(material, resource_type::resource_type_material);

}
