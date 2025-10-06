// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"
#include "mesh_raw.hpp"
#include "primitive_raw.hpp"
#include "primitive.hpp"
#include "memory/chunk_allocator.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_event_storage_gfx.hpp"

namespace SFG
{
	mesh_reflection::mesh_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<mesh>::value, "");
		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<mesh>(MAX_WORLD_MESHES); });
	}

	void mesh::create_from_raw(const mesh_raw& raw, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle)
	{
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);

		_node_index = raw.node_index;
		_skin_index = raw.skin_index;

		render_event ev = {.header = {
							   .handle	   = handle,
							   .event_type = render_event_type::render_event_create_mesh,
						   }};

		render_event_storage_mesh* stg = reinterpret_cast<render_event_storage_mesh*>(ev.data);
		stg->name					   = alloc.get<const char>(_name);
		stg->primitives_static.resize(raw.primitives_static.size());
		stg->primitives_skinned.resize(raw.primitives_skinned.size());

		vector<uint16> materials;

		auto add_material = [&](uint16 m) {
			int32 index = vector_util::index_of(materials, m);
			if (index == -1)
				materials.push_back(m);
		};

		if (!raw.primitives_static.empty())
		{
			const uint32 prims_count = static_cast<uint32>(raw.primitives_static.size());
			for (uint32 i = 0; i < prims_count; i++)
			{
				const primitive_static_raw& prim_loaded = raw.primitives_static[i];
				add_material(prim_loaded.material_index);
			}
		}

		if (!raw.primitives_skinned.empty())
		{
			const uint32 prims_count = static_cast<uint32>(raw.primitives_skinned.size());
			for (uint32 i = 0; i < prims_count; i++)
			{
				const primitive_skinned_raw& prim_loaded = raw.primitives_skinned[i];
				add_material(prim_loaded.material_index);
			}
		}

		if (!raw.primitives_static.empty())
			SFG_MEMCPY(stg->primitives_static.data(), raw.primitives_static.data(), raw.primitives_static.size() * sizeof(primitive_static_raw));

		if (!raw.primitives_skinned.empty())
			SFG_MEMCPY(stg->primitives_skinned.data(), raw.primitives_skinned.data(), raw.primitives_skinned.size() * sizeof(primitive_skinned_raw));
		stream.add_event(ev);

		// run through again to localize material indices.
		if (!raw.primitives_static.empty())
		{
			const uint32 prims_count = static_cast<uint32>(raw.primitives_static.size());
			for (uint32 i = 0; i < prims_count; i++)
			{
				primitive_static_raw& prim_loaded = stg->primitives_static[i];
				prim_loaded.material_index		  = static_cast<uint16>(vector_util::index_of(materials, prim_loaded.material_index));
			}
		}

		if (!raw.primitives_skinned.empty())
		{
			const uint32 prims_count = static_cast<uint32>(raw.primitives_skinned.size());
			for (uint32 i = 0; i < prims_count; i++)
			{
				primitive_skinned_raw& prim_loaded = stg->primitives_skinned[i];
				prim_loaded.material_index		   = static_cast<uint16>(vector_util::index_of(materials, prim_loaded.material_index));
			}
		}

		_material_count	  = static_cast<uint16>(materials.size());
		_material_indices = alloc.allocate<uint16>(materials.size());
		SFG_MEMCPY(alloc.get(_material_indices.head), materials.data(), sizeof(uint16) * materials.size());
	}

	void mesh::destroy(chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle)
	{
		if (_name.size != 0)
			alloc.free(_name);

		alloc.free(_material_indices);

		_name			  = {};
		_material_indices = {};
		_material_count	  = 0;

		stream.add_event({.header = {
							  .handle	  = handle,
							  .event_type = render_event_type::render_event_destroy_mesh,
						  }});
	}
}