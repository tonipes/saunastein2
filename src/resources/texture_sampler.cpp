// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"
#include "texture_sampler_raw.hpp"
#include "math/math_common.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{

	void texture_sampler::create_from_loader(const texture_sampler_raw& raw, world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

		_desc = raw.desc;
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		render_event_sampler stg = {};
		stg.desc				 = raw.desc;

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::create_sampler,
			},
			stg);
	}

	void texture_sampler::destroy(world& w, resource_handle handle)
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
			.event_type = render_event_type::destroy_sampler,
		});
	}

}
