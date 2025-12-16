// Copyright (c) 2025 Inan Evin

#include "comp_light.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{

	void comp_dir_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}

	void comp_dir_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_dir_light,
		});
	}

	void comp_dir_light::send_event(world& w)
	{
		render_event_dir_light ev = {};
		ev.entity_index			  = _header.entity.index;
		ev.base_color			  = vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.intensity			  = _intensity;
		ev.cast_shadows			  = _cast_shadows;
		ev.shadow_resolution	  = _shadow_resolution;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_dir_light,
			},
			ev);
	}

	void comp_dir_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}

	void comp_dir_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

	void comp_dir_light::set_values(world& w, const color& c, float intensity)
	{
		_base_color = c;
		_intensity	= intensity;
		send_event(w);
	}

	void comp_dir_light::set_shadow_values(world& w, uint8 cast_shadows, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_shadow_resolution = resolution;
		send_event(w);
	}

	void comp_spot_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}
	void comp_spot_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_spot_light,
		});
	}

	void comp_spot_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}
	void comp_spot_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

	void comp_spot_light::set_values(world& w, const color& c, float range, float intensity, float inner_cone, float outer_cone)
	{
		_base_color = c;
		_range		= range;
		_intensity	= intensity;
		_inner_cone = inner_cone;
		_outer_cone = outer_cone;
		send_event(w);
	}

	void comp_spot_light::set_shadow_values(world& w, uint8 cast_shadows, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_shadow_resolution = resolution;
		send_event(w);
	}

	void comp_spot_light::send_event(world& w)
	{
		render_event_spot_light ev = {};
		ev.entity_index			   = _header.entity.index;
		ev.base_color			   = vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.range				   = _range;
		ev.intensity			   = _intensity;
		ev.inner_cone			   = _inner_cone;
		ev.outer_cone			   = _outer_cone;
		ev.cast_shadows			   = _cast_shadows;
		ev.shadow_resolution	   = _shadow_resolution;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_spot_light,
			},
			ev);
	}

	void comp_point_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}
	void comp_point_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_point_light,
		});
	}

	void comp_point_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}
	void comp_point_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

	void comp_point_light::set_values(world& w, const color& c, float range, float intensity)
	{
		_base_color = c;
		_range		= range;
		_intensity	= intensity;
		send_event(w);
	}

	void comp_point_light::set_shadow_values(world& w, uint8 cast_shadows, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_shadow_resolution = resolution;
		send_event(w);
	}

	void comp_point_light::send_event(world& w)
	{
		render_event_point_light ev = {};
		ev.entity_index				= _header.entity.index;
		ev.base_color				= vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.range					= _range;
		ev.intensity				= _intensity;
		ev.cast_shadows				= _cast_shadows;
		ev.shadow_resolution		= _shadow_resolution;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_point_light,
			},
			ev);
	}

#ifdef SFG_TOOLMODE

	void comp_dir_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}

	void comp_dir_light::deserialize_json(const nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

	void comp_spot_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}
	void comp_spot_light::deserialize_json(const nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

	void comp_point_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}
	void comp_point_light::deserialize_json(const nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

#endif
}