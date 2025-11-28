// Copyright (c) 2025 Inan Evin

#include "skin.hpp"
#include "skin_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"

#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

namespace SFG
{

	void skin::create_from_loader(const skin_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_root		  = raw.root_joint;
		_joints_count = static_cast<uint16>(raw.joints.size());
		_joints		  = alloc.allocate<skin_joint>(raw.joints.size());

		skin_joint*	 ptr   = reinterpret_cast<skin_joint*>(alloc.get(_joints.head));
		const uint32 count = static_cast<uint32>(raw.joints.size());

		render_event_skin ev = {};
		ev.root_index		 = raw.root_joint;

		for (uint32 i = 0; i < count; i++)
		{
			const skin_joint& joint		   = raw.joints[i];
			const matrix4x4	  inverse_bind = joint.inverse_bind_matrix;
			ptr[i]						   = joint;

			ev.nodes.push_back(joint.model_node_index);
			ev.matrices.push_back(matrix4x3(inverse_bind[0], inverse_bind[1], inverse_bind[2], inverse_bind[4], inverse_bind[5], inverse_bind[6], inverse_bind[8], inverse_bind[9], inverse_bind[10], inverse_bind[12], inverse_bind[13], inverse_bind[14]));
		}

		w.get_render_stream().add_event(
			{
				.index		= handle.index,
				.event_type = render_event_type::create_skin,
			},
			ev);
	}

	void skin::destroy(world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		alloc.free(_joints);

		_joints = {};

		w.get_render_stream().add_event({
			.index		= handle.index,
			.event_type = render_event_type::destroy_skin,
		});
	}

}