// Copyright (c) 2025 Inan Evin

#include "trait_light.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	trait_dir_light_reflection::trait_dir_light_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_dir_light>::value, type_id<trait_dir_light>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_dir_light>(MAX_ENTITIES); });

		m.add_function<void, world&, world_handle, world_handle>("construct_add"_hs, [](world& w, world_handle entity, world_handle own_handle) {
			trait_dir_light& t	 = w.get_entity_manager().get_trait<trait_dir_light>(own_handle);
			t					 = trait_dir_light();
			t._header.entity	 = entity;
			t._header.own_handle = own_handle;
			t.on_add(w);
		});

		m.add_function<void, world&, world_handle>("destruct_remove"_hs, [](world& w, world_handle own_handle) {
			trait_dir_light& t = w.get_entity_manager().get_trait<trait_dir_light>(own_handle);
			t.on_remove(w);
			t.~trait_dir_light();
		});
	}

	trait_point_light_reflection::trait_point_light_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_point_light>::value, type_id<trait_point_light>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_point_light>(MAX_ENTITIES); });

		m.add_function<void, world&, world_handle, world_handle>("construct_add"_hs, [](world& w, world_handle entity, world_handle own_handle) {
			trait_point_light& t = w.get_entity_manager().get_trait<trait_point_light>(own_handle);
			t					 = trait_point_light();
			t._header.entity	 = entity;
			t._header.own_handle = own_handle;
			t.on_add(w);
		});

		m.add_function<void, world&, world_handle>("destruct_remove"_hs, [](world& w, world_handle own_handle) {
			trait_point_light& t = w.get_entity_manager().get_trait<trait_point_light>(own_handle);
			t.on_remove(w);
			t.~trait_point_light();
		});
	}

	trait_spot_light_reflection::trait_spot_light_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_spot_light>::value, type_id<trait_spot_light>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_spot_light>(MAX_ENTITIES); });

		m.add_function<void, world&, world_handle, world_handle>("construct_add"_hs, [](world& w, world_handle entity, world_handle own_handle) {
			trait_spot_light& t	 = w.get_entity_manager().get_trait<trait_spot_light>(own_handle);
			t					 = trait_spot_light();
			t._header.entity	 = entity;
			t._header.own_handle = own_handle;
			t.on_add(w);
		});

		m.add_function<void, world&, world_handle>("destruct_remove"_hs, [](world& w, world_handle own_handle) {
			trait_spot_light& t = w.get_entity_manager().get_trait<trait_spot_light>(own_handle);
			t.on_remove(w);
			t.~trait_spot_light();
		});
	}

	void trait_dir_light::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);

		render_event_dir_light ev = {};
		ev.entity_index			  = _header.entity.index;
		ev.base_color			  = _base_color;
		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::render_event_update_dir_light,
			},
			ev);
	}

	void trait_dir_light::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::render_event_remove_dir_light,
		});
	}

	void trait_spot_light::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);

		render_event_spot_light ev = {};
		ev.entity_index			   = _header.entity.index;
		ev.base_color			   = _base_color;
		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::render_event_update_spot_light,
			},
			ev);
	}
	void trait_spot_light::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::render_event_remove_spot_light,
		});
	}

	void trait_point_light::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);

		render_event_point_light ev = {};
		ev.entity_index				= _header.entity.index;
		ev.base_color				= _base_color;
		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::render_event_update_point_light,
			},
			ev);
	}
	void trait_point_light::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::render_event_remove_point_light,
		});
	}

	void trait_dir_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}

	void trait_dir_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

	void trait_spot_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}
	void trait_spot_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

	void trait_point_light::serialize(ostream& stream, world& w) const
	{
		stream << _base_color;
	}
	void trait_point_light::deserialize(istream& stream, world& w)
	{
		stream >> _base_color;
	}

#ifdef SFG_TOOLMODE

	void trait_dir_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}

	void trait_dir_light::deserialize_json(nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

	void trait_spot_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}
	void trait_spot_light::deserialize_json(nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

	void trait_point_light::serialize_json(nlohmann::json& j, world& w) const
	{
		j["base_color"] = _base_color;
	}
	void trait_point_light::deserialize_json(nlohmann::json& j, world& w)
	{
		_base_color = j.value<color>("base_color", color::white);
	}

#endif
}