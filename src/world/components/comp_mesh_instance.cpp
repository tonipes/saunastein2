// Copyright (c) 2025 Inan Evin
#include "comp_mesh_instance.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void comp_mesh_instance::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
	}

	void comp_mesh_instance::on_remove(world& w)
	{
		chunk_allocator32& aux = w.get_comp_manager().get_aux();
		if (_skin_entities.size != 0)
			aux.free(_skin_entities);
		_skin_entities = {};

		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_mesh_instance,
		});
	}

	void comp_mesh_instance::set_mesh(world& w, resource_handle model_handle, resource_handle mesh, resource_handle skin, world_handle* skin_node_entities, uint16 skin_node_entity_count)
	{
		_target_model = model_handle;
		_target_mesh  = mesh;

		render_event_mesh_instance stg = {};
		stg.entity_index			   = _header.entity.index;
		stg.model					   = _target_model.index;
		stg.mesh					   = _target_mesh.index;
		stg.skin					   = skin.is_null() ? NULL_RESOURCE_ID : skin.index;

		for (uint16 i = 0; i < skin_node_entity_count; i++)
		{
			stg.skin_node_entities.push_back(skin_node_entities[i].index);
		}

		chunk_allocator32& aux = w.get_comp_manager().get_aux();

		if (_skin_entities.size != 0)
			aux.free(_skin_entities);
		_skin_entities		 = {};
		_skin_entities_count = skin_node_entity_count;

		if (_skin_entities_count != 0)
		{
			_skin_entities	  = aux.allocate<world_handle>(_skin_entities_count);
			world_handle* ptr = aux.get<world_handle>(_skin_entities);

			for (uint16 i = 0; i < _skin_entities_count; i++)
				ptr[i] = skin_node_entities[i];
		}

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_mesh_instance,
			},
			stg);
	}

	void comp_mesh_instance::serialize(ostream& stream, world& w) const
	{
		resource_manager& rm = w.get_resource_manager();

		string_id		  target_model_hash = 0;
		string_id		  target_mesh_hash	= 0;
		vector<string_id> target_materials	= {};
		fetch_refs(rm, target_model_hash, target_mesh_hash);

		stream << target_model_hash;
		stream << target_mesh_hash;
	}

	void comp_mesh_instance::deserialize(istream& stream, world& w)
	{
		string_id target_model_hash = 0;
		string_id target_mesh_hash	= 0;
		stream >> target_model_hash;
		stream >> target_mesh_hash;
		fill_refs(w.get_resource_manager(), target_model_hash, target_mesh_hash);
	}

#ifdef SFG_TOOLMODE

	void comp_mesh_instance::serialize_json(nlohmann::json& j, world& w) const
	{
		resource_manager& rm = w.get_resource_manager();

		string_id		  target_model_hash = 0;
		string_id		  target_mesh_hash	= 0;
		vector<string_id> target_materials	= {};
		fetch_refs(rm, target_model_hash, target_mesh_hash);

		j["target_model"] = target_model_hash;
		j["target_mesh"]  = target_mesh_hash;
	}

	void comp_mesh_instance::deserialize_json(const nlohmann::json& j, world& w)
	{
		resource_manager& rm = w.get_resource_manager();

		const string_id target_model_hash = j.value<string_id>("target_model", 0);
		const string_id target_mesh_hash  = j.value<string_id>("target_mesh", 0);
		fill_refs(w.get_resource_manager(), target_model_hash, target_mesh_hash);
	}
#endif

	void comp_mesh_instance::fetch_refs(resource_manager& rm, string_id& out_target, string_id& out_target_mesh) const
	{
		out_target		= rm.get_resource_hash<model>(_target_model);
		out_target_mesh = rm.get_resource_hash<mesh>(_target_mesh);
	}

	void comp_mesh_instance::fill_refs(resource_manager& rm, string_id target_model, string_id target_mesh)
	{
		_target_model = rm.get_resource_handle_by_hash<model>(target_model);
		_target_mesh  = rm.get_resource_handle_by_hash<mesh>(target_mesh);
	}

}