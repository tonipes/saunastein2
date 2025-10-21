// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"
#include "mesh_raw.hpp"
#include "primitive_raw.hpp"
#include "primitive.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

namespace SFG
{

	void mesh::create_from_loader(const mesh_raw& raw, world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_node_index = raw.node_index;
		_skin_index = raw.skin_index;
		_sid		= raw.sid;

		render_event_mesh ev  = {};
		ev.primitives_static  = raw.primitives_static;
		ev.primitives_skinned = raw.primitives_skinned;
		ev.local_aabb		  = raw.local_aabb;

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

	void mesh::destroy(world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_destroy_mesh,
		});

		_node_index = _skin_index = _sid = 0;
	}
}