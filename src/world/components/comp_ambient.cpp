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

#include "comp_ambient.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{

	void comp_ambient::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_ambient ev = {
			.base_color	  = vector3(_base_color.x, _base_color.y, _base_color.z),
			.entity_index = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_ambient,
			},
			ev);
	}

	void comp_ambient::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_ambient,
		});
	}

	void comp_ambient::set_values(world& w, const color& base_color)
	{
		_base_color = base_color;

		const render_event_ambient ev = {
			.base_color	  = vector3(_base_color.x, _base_color.y, _base_color.z),
			.entity_index = _header.entity.index,

		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_ambient,
			},
			ev);
	}

	void comp_ambient::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}

	void comp_ambient::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

#ifdef SFG_TOOLMODE

	void comp_ambient::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}

	void comp_ambient::deserialize_json(const nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

#endif
}