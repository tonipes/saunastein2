// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "shader_raw.hpp"
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{

	void shader::create_from_loader(const shader_raw& raw, world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		render_event_shader ev = {};
		ev.compile_variants	   = raw.compile_variants;
		ev.pso_variants		   = raw.pso_variants;
		ev.layout			   = renderer::get_bind_layout_global();

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::render_event_create_shader,
			},
			ev);
	}

	void shader::destroy(world& w, resource_handle handle)
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
			.event_type = render_event_type::render_event_destroy_shader,
		});
	}
}
