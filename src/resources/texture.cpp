// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "texture_raw.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	texture::~texture()
	{
	}

	void texture::create_from_loader(const texture_raw& raw, world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

		_texture_format = raw.texture_format;
		SFG_ASSERT(!raw.buffers.empty());

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		gfx_backend* backend	= gfx_backend::get();
		uint32		 total_size = 0;
		for (const texture_buffer& buf : raw.buffers)
			total_size += backend->get_texture_size(buf.size.x, buf.size.y, buf.bpp);

		render_event_texture stg = {};
		stg.buffers				 = raw.buffers;
		stg.format				 = _texture_format;
		stg.size				 = raw.buffers[0].size;
		stg.intermediate_size	 = backend->align_texture_size(total_size);

#ifndef SFG_STRIP_DEBUG_NAMES
		stg.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::create_texture,
			},
			stg);
	}

	void texture::destroy(world& w, resource_handle handle)
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
			.event_type = render_event_type::destroy_texture,
		});
	}

}