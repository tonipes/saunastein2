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
		meta& m = reflection::get().register_meta(type_id<mesh>::value, type_id<mesh>::index, "");
		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<mesh>(MAX_WORLD_MESHES); });
	}

	void mesh::create_from_raw(const mesh_raw& raw, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_node_index = raw.node_index;
		_skin_index = raw.skin_index;

		render_event ev = {
			.header =
				{
					.index		= static_cast<uint32>(handle.index),
					.event_type = render_event_type::render_event_create_mesh,
				},
		};

		render_event_storage_mesh* stg = ev.construct<render_event_storage_mesh>();

#ifndef SFG_STRIP_DEBUG_NAMES
		stg->name = reinterpret_cast<const char*>(SFG_MALLOC(_name.size));
		if (stg->name)
			strcpy((char*)stg->name, alloc.get<const char>(_name));
#endif

		stg->primitives_static.resize(raw.primitives_static.size());
		stg->primitives_skinned.resize(raw.primitives_skinned.size());

		uint32 i = 0;
		for (const primitive_static_raw& r : raw.primitives_static)
		{
			stg->primitives_static[i] = r;
			i++;
		}

		i = 0;
		for (const primitive_skinned_raw& r : raw.primitives_skinned)
		{
			stg->primitives_skinned[i] = r;
			i++;
		}

		stream.add_event(ev);
	}

	void mesh::destroy(chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.header =
				{
					.index		= static_cast<uint32>(handle.index),
					.event_type = render_event_type::render_event_destroy_mesh,
				},
		});
	}
}