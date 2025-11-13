// Copyright (c) 2025 Inan Evin
#include "trait_camera.hpp"
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

	void trait_camera::set_values(world& w, float near_plane, float far_plane, float fov_degrees, std::initializer_list<float> cascades)
	{
		_near		 = near_plane;
		_far		 = far_plane;
		_fov_degrees = fov_degrees;
		_cascades	 = cascades;

		const render_event_camera ev = {
			.cascades	  = _cascades,
			.entity_index = _header.entity.index,
			.near_plane	  = _near,
			.far_plane	  = _far,
			.fov_degrees  = _fov_degrees,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_camera,
			},
			ev);
	}

	void trait_camera::set_main(world& w)
	{
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::set_main_camera,
		});
	}

	void trait_camera::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);

		const render_event_camera ev = {
			.entity_index = _header.entity.index,
			.near_plane	  = _near,
			.far_plane	  = _far,
			.fov_degrees  = _fov_degrees,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_camera,
			},
			ev);
	}

	void trait_camera::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_camera,
		});
	}

	void trait_camera::serialize(ostream& stream, world& w) const
	{
		stream << _near;
		stream << _far;
		stream << _fov_degrees;
	}

	void trait_camera::deserialize(istream& stream, world& w)
	{
		stream >> _near;
		stream >> _far;
		stream >> _fov_degrees;
	}

#ifdef SFG_TOOLMODE

	void trait_camera::serialize_json(nlohmann::json& j, world& w) const
	{
		j["near"]		 = _near;
		j["far"]		 = _far;
		j["fov_degrees"] = _fov_degrees;
	}

	void trait_camera::deserialize_json(const nlohmann::json& j, world& w)
	{
		_near		 = j.value<float>("near", 0.0f);
		_far		 = j.value<float>("far", 0.0f);
		_fov_degrees = j.value<float>("fov_degrees", 0.0f);
	}

#endif
}