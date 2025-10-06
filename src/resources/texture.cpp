// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "texture_raw.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/world/world_renderer.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_event_storage_gfx.hpp"

namespace SFG
{
	texture_reflection g_texture_reflection = {};

	texture_reflection::texture_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<texture>::value, "stkphy");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			texture_raw* raw = new texture_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			texture_raw* raw = new texture_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			texture_raw*		 raw_ptr   = reinterpret_cast<texture_raw*>(raw);
			world_resources&	 resources = w.get_resources();
			resource_handle		 handle	   = resources.add_resource<texture>(sid);
			texture&			 res	   = resources.get_resource<texture>(handle);
			render_event_stream& stream	   = w.get_render_stream();
			res.create_from_raw(*raw_ptr, stream, resources.get_aux(), handle);
			delete raw_ptr;

			return handle;
		});

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& resources = w.get_resources();
			texture&		 txt	   = resources.get_resource<texture>(h);
			txt.destroy(w.get_render_stream(), resources.get_aux(), h);
			resources.remove_resource<texture>(h);
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<texture>(MAX_WORLD_TEXTURES); });

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			texture_raw* raw = reinterpret_cast<texture_raw*>(loader);
			raw->serialize(stream);
		});
	}

	texture::~texture()
	{
	}

	void texture::create_from_raw(const texture_raw& raw, render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
		_texture_format = raw.texture_format;
		SFG_ASSERT(raw.buffers.empty());

		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);

		render_event ev = {
			.header =
				{
					.handle		= handle,
					.event_type = render_event_type::render_event_create_texture,
				},
		};

		gfx_backend* backend	= gfx_backend::get();
		uint32		 total_size = 0;
		for (const texture_buffer& buf : raw.buffers)
			total_size += backend->get_texture_size(buf.size.x, buf.size.y, buf.bpp);

		render_event_storage_texture* stg = reinterpret_cast<render_event_storage_texture*>(ev.data);
		stg->buffers					  = raw.buffers;
		stg->format						  = _texture_format;
		stg->size						  = raw.buffers[0].size;
		stg->name						  = alloc.get<const char>(_name);
		stg->intermediate_size			  = backend->align_texture_size(total_size);
		if (stg->name != nullptr)
			strcpy((char*)stg->name, raw.name.c_str());
		stream.add_event(ev);
	}

	void texture::destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
		if (_name.size != 0)
			alloc.free(_name);

		stream.add_event({.header = {
							  .handle	  = handle,
							  .event_type = render_event_type::render_event_destroy_texture,
						  }});

		_name = {};
	}

}