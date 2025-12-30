/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "material.hpp"
#include "material_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	material::~material()
	{
		SFG_ASSERT(!_flags.is_set(material_flags::material_flags_created));
	}

	void material::create_from_loader(const material_raw& raw, world& w, resource_handle handle, resource_handle sampler)
	{
		SFG_ASSERT(!_flags.is_set(material_flags::material_flags_created));
		_flags.set(material_flags::material_flags_created);

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
		_flags.set(material_flags::material_flags_is_forward, raw.pass_mode == material_pass_mode::forward);
		_flags.set(material_flags::material_flags_is_alpha_cutoff, raw.use_alpha_cutoff);
		_flags.set(material_flags::material_flags_is_double_sided, raw.double_sided);
		_flags.set(material_flags::material_flags_is_gui, raw.pass_mode == material_pass_mode::gui);
		SFG_ASSERT(raw.shader != 0);

		render_event_material stg = {};
		stg.data.data			  = reinterpret_cast<uint8*>(SFG_MALLOC(_material_data.get_size()));

		if (stg.data.data)
			SFG_MEMCPY(stg.data.data, _material_data.get_raw(), _material_data.get_size());
		stg.data.size = _material_data.get_size();
		stg.flags	  = _flags.value();
		stg.priority  = raw.draw_priority;
		stg.sampler	  = sampler;

		for (string_id sid : raw.textures)
		{
			const resource_handle handle = rm.get_resource_handle_by_hash<texture>(sid);
			stg.textures.push_back(handle);
		}
		const resource_handle shader_handle = rm.get_resource_handle_by_hash<shader>(raw.shader);
		stg.shader_index					= shader_handle.index;

#ifndef SFG_STRIP_DEBUG_NAMES
		stg.name = raw.name;
#endif
		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::create_material,
			},
			stg);
	}

	void material::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(material_flags::material_flags_created))
			return;
		_flags.remove(material_flags::material_flags_created);

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
			.event_type = render_event_type::destroy_material,
		});

		_material_data.destroy();
	}

	void material::update_data(world& w, resource_handle handle)
	{
		w.get_render_stream().add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::update_material,
		});
	}
	void material::update_sampler(world& w, resource_handle material_own_handle, resource_handle sampler)
	{
		render_event_update_material_sampler ev = {};
		ev.sampler								= sampler.index;

		w.get_render_stream().add_event(
			{
				.index		= static_cast<uint32>(material_own_handle.index),
				.event_type = render_event_type::update_material_sampler,
			},
			ev);
	}
}