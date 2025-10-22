// Copyright (c) 2025 Inan Evin
#include "trait_model_instance.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "world/traits/trait_mesh_instance.hpp"
#include "world/traits/trait_light.hpp"
#include "resources/model.hpp"
#include "resources/model_node.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "resources/light_raw.hpp"
#include "io/log.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{

	void trait_model_instance::on_add(world& w)
	{
	}

	void trait_model_instance::on_remove(world& w)
	{
		trait_manager&	   tm  = w.get_trait_manager();
		chunk_allocator32& aux = tm.get_aux();

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
		trait_manager&	   tm	   = w.get_trait_manager();
		resource_manager&  res	   = w.get_resource_manager();
		chunk_allocator32& em_aux  = tm.get_aux();
		chunk_allocator32& res_aux = res.get_aux();

		model& mdl = res.get_resource<model>(model_handle);
		if (mdl.get_material_count() == 0 && materials == nullptr)
		{
			SFG_ERR("No materials are provided for given model, this is not supported!");
			return;
		}

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

		const uint16 lights_count = mdl.get_light_count();
		light_raw*	 lights_ptr	  = nullptr;
		if (lights_count != 0)
			lights_ptr = res_aux.get<light_raw>(mdl.get_created_lights());

		// add components.
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node&		   node		   = ptr_nodes[i];
			const world_handle entity	   = created_node_entities[i];
			const int16		   light_index = node.get_light_index();
			if (lights_ptr && light_index != -1)
			{
				SFG_ASSERT(light_index < static_cast<int16>(lights_count));
				light_raw& lr = lights_ptr[light_index];

				if (lr.type == light_raw_type::point)
				{
					const world_handle light_handle = tm.add_trait<trait_point_light>(entity);
					trait_point_light& trait_light	= tm.get_trait<trait_point_light>(light_handle);
					trait_light.set_values(w, lr.base_color, lr.range, lr.intensity);
				}
				else if (lr.type == light_raw_type::spot)
				{
					const world_handle light_handle = tm.add_trait<trait_spot_light>(entity);
					trait_spot_light&  trait_light	= tm.get_trait<trait_spot_light>(light_handle);
					trait_light.set_values(w, lr.base_color, lr.range, lr.intensity, lr.outer_cone, lr.inner_cone);
				}
				else if (lr.type == light_raw_type::sun)
				{
					const world_handle light_handle = tm.add_trait<trait_dir_light>(entity);
					trait_dir_light&   trait_light	= tm.get_trait<trait_dir_light>(light_handle);
					trait_light.set_values(w, lr.base_color, lr.range, lr.intensity);
				}
			}

			if (node.get_mesh_index() == -1)
				continue;

			const resource_handle& mesh_handle	= ptr_meshes_handle[node.get_mesh_index()];
			const mesh&			   m			= res.get_resource<mesh>(mesh_handle);
			const world_handle	   trait_handle = tm.add_trait<trait_mesh_instance>(entity);
			trait_mesh_instance&   mi			= tm.get_trait<trait_mesh_instance>(trait_handle);
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
		resource_manager& rm = w.get_resource_manager();

		string_id target_model_hash = 0;
		fetch_refs(rm, target_model_hash);

		stream << target_model_hash;
	}

	void trait_model_instance::deserialize(istream& stream, world& w)
	{
		string_id target_model_hash = 0;
		stream >> target_model_hash;
		fill_refs(w.get_resource_manager(), target_model_hash);
	}

#ifdef SFG_TOOLMODE

	void trait_model_instance::serialize_json(nlohmann::json& j, world& w) const
	{
		resource_manager& rm = w.get_resource_manager();

		string_id target_model_hash = 0;
		fetch_refs(rm, target_model_hash);

		j["target_model"] = target_model_hash;
	}

	void trait_model_instance::deserialize_json(const nlohmann::json& j, world& w)
	{
		resource_manager& rm = w.get_resource_manager();

		const string_id target_model_hash = j.value<string_id>("target_model", 0);
		fill_refs(w.get_resource_manager(), target_model_hash);
	}
#endif

	void trait_model_instance::fetch_refs(resource_manager& rm, string_id& out_target) const
	{
		out_target = rm.get_resource_hash<model>(_target_model);
	}

	void trait_model_instance::fill_refs(resource_manager& rm, string_id target_model)
	{
		_target_model = rm.get_resource_handle_by_hash<model>(target_model);
	}

}