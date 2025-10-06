// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"
#include "texture_sampler_raw.hpp"
#include "math/math_common.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_event_storage_gfx.hpp"
#include "resource_reflection_template.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	texture_sampler_reflection g_texture_sampler_reflection = {};

	texture_sampler_reflection::texture_sampler_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<texture_sampler>::value, "stksampler");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*, world&>("cook_from_file"_hs, [](const char* path, world& w) -> void* {
			texture_sampler_raw* raw = new texture_sampler_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			world_resources&				 resources = w.get_resources();
			world_resources::resource_watch& watch	   = resources.add_resource_watch();
			watch.base_path							   = path;
			watch.dependencies.push_back(engine_data::get().get_working_dir() + raw->name);

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			texture_sampler_raw* raw = new texture_sampler_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&>("create_from_raw"_hs, [](void* raw, world& w) -> resource_handle {
			texture_sampler_raw* raw_ptr   = reinterpret_cast<texture_sampler_raw*>(raw);
			world_resources&	 resources = w.get_resources();
			resource_handle		 handle	   = resources.add_resource<texture_sampler>(TO_SID(raw_ptr->name));
			texture_sampler&	 res	   = resources.get_resource<texture_sampler>(handle);
			res.create_from_raw(*raw_ptr, w.get_render_stream(), resources.get_aux(), handle);
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<texture_sampler>(MAX_WORLD_SAMPLERS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle handle) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<texture_sampler>(handle).destroy(w.get_render_stream(), res.get_aux(), handle);
			res.remove_resource<texture_sampler>(handle);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			texture_sampler_raw* raw = reinterpret_cast<texture_sampler_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void texture_sampler::create_from_raw(const texture_sampler_raw& raw, render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);

		render_event ev = {
			.header =
				{
					.handle		= handle,
					.event_type = render_event_type::render_event_create_sampler,
				},
		};
		render_event_storage_sampler* stg = reinterpret_cast<render_event_storage_sampler*>(ev.data);
		stg->desc						  = raw.desc;
		stg->desc.debug_name			  = reinterpret_cast<const char*>(SFG_MALLOC(_name.size));
		if (stg->desc.debug_name)
			strcpy((char*)stg->desc.debug_name, alloc.get<const char>(_name));

		stream.add_event(ev);
	}

	void texture_sampler::destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
		if (_name.size != 0)
			alloc.free(_name);

		render_event ev = {
			.header =
				{
					.handle		= handle,
					.event_type = render_event_type::render_event_destroy_sampler,
				},
		};
		stream.add_event(ev);

		_name = {};
	}

}
