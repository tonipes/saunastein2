// Copyright (c) 2025 Inan Evin
#include "trait_model_instance.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "reflection/reflection.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	trait_model_instance_reflection::trait_model_instance_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_model_instance>::value, type_id<trait_model_instance>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_model_instance>(MAX_MODEL_INSTANCES); });
	}

	void trait_model_instance::on_add(world& w, trait_handle handle, entity_handle entity)
	{
		_header.entity = entity;
		w.get_entity_manager().on_add_render_proxy(_header.entity);
	}

	void trait_model_instance::on_remove(world& w, trait_handle handle)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);
	}

	void trait_model_instance::set_model(resource_handle handle, world& w, render_event_stream& stream, resource_handle target_mesh)
	{
		world_resources&   resources = w.get_resources();
		chunk_allocator32& aux		 = resources.get_aux();
		model&			   mdl		 = resources.get_resource<model>(handle);
		_target_model				 = handle;
		_material_count				 = mdl.get_material_count();
		_target_mesh				 = target_mesh;
		if (_material_count != 0)
			_materials = mdl.get_created_materials();

		render_event_model_instance stg = {};

		stg.model		= _target_model.index;
		stg.single_mesh = !_target_mesh.is_null();
		stg.materials.resize(_material_count);

		resource_handle* materials = aux.get<resource_handle>(_materials);
		for (uint16 i = 0; i < _material_count; i++)
			stg.materials[i] = materials[i].index;

		stream.add_event(
			{
				.index		= _header.entity.index,
				.event_type = render_event_type::render_event_update_model_instance,
			},
			stg);
	}

	void trait_model_instance::set_model(string_id sid, world& w, render_event_stream& stream, resource_handle target_mesh)
	{
		const resource_handle handle = w.get_resources().get_resource_handle_by_hash<model>(sid);
		set_model(handle, w, stream);
	}

	void trait_model_instance::serialize(ostream& stream, world& w) const
	{
		world_resources& resources = w.get_resources();

		string_id		  target_model_hash = 0;
		string_id		  target_mesh_hash	= 0;
		vector<string_id> target_materials	= {};
		fetch_refs(resources, target_model_hash, target_mesh_hash, target_materials);

		stream << target_model_hash;
		stream << target_mesh_hash;
		stream << _material_count;
		stream << target_materials;
	}

	void trait_model_instance::deserialize(istream& stream, world& w)
	{
		string_id		  target_model_hash = 0;
		string_id		  target_mesh_hash	= 0;
		vector<string_id> target_materials	= {};
		stream >> target_model_hash;
		stream >> target_mesh_hash;
		stream >> _material_count;
		stream >> target_materials;
		SFG_ASSERT(static_cast<uint16>(target_materials.size()) == _material_count);
		fill_refs(w.get_resources(), target_model_hash, target_mesh_hash, target_materials);
	}

#ifdef SFG_TOOLMODE

	void trait_model_instance::serialize_json(nlohmann::json& j, world& w) const
	{
		world_resources& resources = w.get_resources();

		string_id		  target_model_hash = 0;
		string_id		  target_mesh_hash	= 0;
		vector<string_id> target_materials	= {};
		fetch_refs(resources, target_model_hash, target_mesh_hash, target_materials);

		j["target_model"]	= target_model_hash;
		j["target_mesh"]	= target_mesh_hash;
		j["material_count"] = _material_count;
		j["materials"]		= target_materials;
	}

	void trait_model_instance::deserialize_json(nlohmann::json& j, world& w)
	{
		world_resources& resources = w.get_resources();

		const string_id			target_model_hash = j.value<string_id>("target_model", 0);
		const string_id			target_mesh_hash  = j.value<string_id>("target_mesh", 0);
		const vector<string_id> target_materials  = j.value<vector<string_id>>("materials", {});

		_material_count = j.value<uint16>("material_count", 0);
		SFG_ASSERT(static_cast<uint16>(target_materials.size()) == _material_count);
		fill_refs(w.get_resources(), target_model_hash, target_mesh_hash, target_materials);
	}

#endif

	void trait_model_instance::fetch_refs(world_resources& resources, string_id& out_target, string_id& out_target_mesh, vector<string_id>& out_materials) const
	{
		out_target = resources.get_resource_hash<model>(_target_model);
		out_materials.resize(_material_count);
		resource_handle* handles = resources.get_aux().get<resource_handle>(_materials);
		for (uint16 i = 0; i < _material_count; i++)
		{
			const resource_handle& h	= handles[i];
			const string_id		   hash = resources.get_resource_hash<material>(h);
			out_materials[i]			= hash;
		}
	}

	void trait_model_instance::fill_refs(world_resources& resources, string_id target_model, string_id target_mesh, const vector<string_id>& materials)
	{
		_target_model = resources.get_resource_handle_by_hash<model>(target_model);

		if (resources.is_valid<mesh>(target_mesh))
			_target_mesh = resources.get_resource_handle_by_hash<mesh>(target_mesh);

		chunk_allocator32& aux = resources.get_aux();
		if (_material_count != 0)
			_materials = aux.allocate<resource_handle>(_material_count);
		resource_handle* handles = aux.get<resource_handle>(_materials);

		for (uint16 i = 0; i < _material_count; i++)
		{
			const string_id sid = materials.at(i);
			handles[i]			= resources.get_resource_handle_by_hash<material>(sid);
		}
	}

}