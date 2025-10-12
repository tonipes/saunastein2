// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"
#include "mesh_raw.hpp"
#include "primitive_raw.hpp"
#include "primitive.hpp"
#include "memory/chunk_allocator.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

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

		render_event_mesh ev  = {};
		ev.primitives_static  = raw.primitives_static;
		ev.primitives_skinned = raw.primitives_skinned;

#ifndef SFG_STRIP_DEBUG_NAMES
		ev.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::render_event_create_mesh,
			},
			ev);
	}

	void mesh::destroy(chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_destroy_mesh,
		});
	}
}