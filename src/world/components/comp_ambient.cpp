// Copyright (c) 2025 Inan Evin
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