// Copyright (c) 2025 Inan Evin

#include "material.hpp"
#include "material_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "gfx/world/world_renderer.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{

	void material::create_from_loader(const material_raw& raw, world& w, resource_handle handle)
	{
		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif
		gfx_backend* backend = gfx_backend::get();

		const uint8 texture_count = static_cast<uint8>(raw.textures.size());
		_material_data			  = raw.material_data;
		_flags.set(material_flags::material_flags_is_gbuffer, raw.pass_mode == material_pass_mode::gbuffer);
		_flags.set(material_flags::material_flags_is_gbuffer_discard, raw.pass_mode == material_pass_mode::gbuffer_transparent);
		_flags.set(material_flags::material_flags_is_forward, raw.pass_mode == material_pass_mode::forward);
		SFG_ASSERT(!raw.shaders.empty());

		render_event_material stg = {};
		stg.data.data			  = reinterpret_cast<uint8*>(SFG_MALLOC(_material_data.get_size()));
		SFG_MEMCPY(stg.data.data, _material_data.get_raw(), _material_data.get_size());
		stg.data.size	  = _material_data.get_size();
		stg.flags		  = _flags.value();
		// stg.use_sampler	  = !sampler_handle.is_null();
		//stg.sampler_index = sampler_handle.index;

		for (string_id sid : raw.textures)
		{
			const resource_handle handle = rm.get_resource_handle_by_hash<texture>(sid);
			stg.textures.push_back(handle);
		}

		for (string_id sid : raw.shaders)
		{
			const resource_handle handle = rm.get_resource_handle_by_hash<shader>(sid);
			stg.shaders.push_back(handle);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stg.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::render_event_create_material,
			},
			stg);
	}

	void material::destroy(world& w, resource_handle handle)
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
			.event_type = render_event_type::render_event_destroy_material,
		});

		_material_data.destroy();
	}

	void material::update_data(world& w, resource_handle handle)
	{
		w.get_render_stream().add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_update_material,
		});
	}
}