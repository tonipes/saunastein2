// Copyright (c) 2025 Inan Evin
#include "trait_model_instance.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "world/traits/trait_mesh_instance.hpp"
#include "resources/model.hpp"
#include "resources/model_node.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "reflection/reflection.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	trait_model_instance_reflection::trait_model_instance_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_model_instance>::value, type_id<trait_model_instance>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_model_instance>(MAX_ENTITIES); });

		m.add_function<void, world&, world_handle, world_handle>("construct_add"_hs, [](world& w, world_handle entity, world_handle own_handle) {
			trait_model_instance& t = w.get_entity_manager().get_trait<trait_model_instance>(own_handle);
			t._header.entity		= entity;
			t._header.own_handle	= own_handle;
			t						= trait_model_instance();
			t.on_add(w);
		});

		m.add_function<void, world&, world_handle>("destruct_remove"_hs, [](world& w, world_handle own_handle) {
			trait_model_instance& t = w.get_entity_manager().get_trait<trait_model_instance>(own_handle);
			t.on_remove(w);
			t.~trait_model_instance();
		});
	}

	void trait_model_instance::on_add(world& w)
	{
	}

	void trait_model_instance::on_remove(world& w)
	{
		chunk_allocator32& aux = w.get_entity_manager().get_traits_aux_memory();

		if (_root_entities.size != 0)
			aux.free(_root_entities);
		_root_entities_count = 0;
	}

	void trait_model_instance::set_model(resource_handle model_handle)
	{
		_target_model = model_handle;
	}

	void trait_model_instance::instantiate_model_to_world(world& w, resource_handle model_handle, resource_handle* materials, uint32 materials_count)
	{
		set_model(_target_model = model_handle);

		entity_manager&	   em	   = w.get_entity_manager();
		world_resources&   res	   = w.get_resources();
		chunk_allocator32& em_aux  = em.get_traits_aux_memory();
		chunk_allocator32& res_aux = res.get_aux();

		// Destroy all entities spawned for this previously.
		if (_root_entities_count != 0)
		{
			world_handle* root_entities_ptr = em_aux.get<world_handle>(_root_entities);
			for (uint32 i = 0; i < _root_entities_count; i++)
				em.destroy_entity(root_entities_ptr[i]);

			if (_root_entities.size != 0)
				em_aux.free(_root_entities);
			_root_entities_count = 0;
		}

		model& mdl = res.get_resource<model>(model_handle);

		const chunk_handle32 meshes		  = mdl.get_created_meshes();
		const chunk_handle32 nodes		  = mdl.get_created_nodes();
		const uint16		 meshes_count = mdl.get_mesh_count();
		const uint16		 nodes_count  = mdl.get_node_count();

		if (nodes_count == 0 || meshes_count == 0)
			return;

		model_node*		 ptr_nodes		   = res_aux.get<model_node>(nodes);
		resource_handle* ptr_meshes_handle = res_aux.get<resource_handle>(meshes);

		// create nodes.
		vector<world_handle> created_node_entities;
		vector<world_handle> root_entities;
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node&		   node = ptr_nodes[i];
			const char*		   name = reinterpret_cast<const char*>(res_aux.get(node.get_name().head));
			const world_handle e	= em.create_entity(name);
			created_node_entities.push_back(e);
			if (node.get_parent_index() == -1)
				root_entities.push_back(e);
		}

		// Store root entities.
		world_handle* root_entities_ptr = nullptr;
		_root_entities_count			= static_cast<uint32>(root_entities.size());
		_root_entities					= em_aux.allocate<world_handle>(root_entities.size(), root_entities_ptr);
		for (uint32 i = 0; i < _root_entities_count; i++)
			root_entities_ptr[i] = root_entities[i];

		// assign parent child.
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_parent_index() != -1)
				em.add_child(created_node_entities[node.get_parent_index()], created_node_entities[i]);
		}

		// assign transform hierarchy
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node	  = ptr_nodes[i];
			vector3		out_pos	  = vector3::zero;
			quat		out_rot	  = quat::identity;
			vector3		out_scale = vector3::zero;
			node.get_local_matrix().decompose(out_pos, out_rot, out_scale);
			em.set_entity_position(created_node_entities[i], out_pos);
			em.set_entity_rotation(created_node_entities[i], out_rot);
			em.set_entity_scale(created_node_entities[i], out_scale);
		}

		// add meshes.
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_mesh_index() == -1)
				continue;

			const resource_handle& mesh_handle	= ptr_meshes_handle[node.get_mesh_index()];
			const mesh&			   m			= res.get_resource<mesh>(mesh_handle);
			const world_handle	   entity		= created_node_entities[i];
			const world_handle	   trait_handle = em.add_trait<trait_mesh_instance>(entity);
			trait_mesh_instance&   mi			= em.get_trait<trait_mesh_instance>(trait_handle);
			mi.set_mesh(w, model_handle, mesh_handle);
		}

		auto report_entity = [&em](world_handle e) {
			const entity_meta& meta	 = em.get_entity_meta(e);
			const vector3&	   pos	 = em.get_entity_position(e);
			const quat&		   rot	 = em.get_entity_rotation(e);
			const vector3&	   scale = em.get_entity_scale(e);
			SFG_INFO("entity: {0}", meta.name);
			SFG_INFO("position: {0} {1} {2}", pos.x, pos.y, pos.z);
			SFG_INFO("rotation: {0} {1} {2} {3}", rot.x, rot.y, rot.z, rot.w);
			SFG_INFO("scale: {0} {1} {2}", scale.x, scale.y, scale.z);
		};
		/*
		for (world_handle e : root_entities)
		{
			SFG_INFO("reporting root...");
			report_entity(e);
			SFG_INFO("reporting children...");
			emr.visit_children(e, [&](world_handle c) { report_entity(c); });
		}
		*/

		for (world_handle r : root_entities)
		{
			em.add_child(_header.entity, r);
		}
	}

	void trait_model_instance::serialize(ostream& stream, world& w) const
	{
		world_resources& resources = w.get_resources();

		string_id target_model_hash = 0;
		fetch_refs(resources, target_model_hash);

		stream << target_model_hash;
	}

	void trait_model_instance::deserialize(istream& stream, world& w)
	{
		string_id target_model_hash = 0;
		stream >> target_model_hash;
		fill_refs(w.get_resources(), target_model_hash);
	}

#ifdef SFG_TOOLMODE

	void trait_model_instance::serialize_json(nlohmann::json& j, world& w) const
	{
		world_resources& resources = w.get_resources();

		string_id target_model_hash = 0;
		fetch_refs(resources, target_model_hash);

		j["target_model"] = target_model_hash;
	}

	void trait_model_instance::deserialize_json(nlohmann::json& j, world& w)
	{
		world_resources& resources = w.get_resources();

		const string_id target_model_hash = j.value<string_id>("target_model", 0);
		fill_refs(w.get_resources(), target_model_hash);
	}
#endif

	void trait_model_instance::fetch_refs(world_resources& resources, string_id& out_target) const
	{
		out_target = resources.get_resource_hash<model>(_target_model);
	}

	void trait_model_instance::fill_refs(world_resources& resources, string_id target_model)
	{
		_target_model = resources.get_resource_handle_by_hash<model>(target_model);
	}

}