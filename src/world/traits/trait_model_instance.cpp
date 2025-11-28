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
#include "resources/skin.hpp"
#include "resources/common_skin.hpp"
#include "io/log.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{

	void trait_model_instance::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
	}

	void trait_model_instance::on_remove(world& w)
	{
		entity_manager& em = w.get_entity_manager();
		em.remove_render_proxy(_header.entity);

		trait_manager&	   tm  = w.get_trait_manager();
		chunk_allocator32& aux = tm.get_aux();

		if (_skin_entities.size != 0)
		{
			const world_handle* skin_entities_ptr = aux.get<world_handle>(_skin_entities);
			for (uint16 i = 0; i < _skin_entities_count; i++)
			{
				em.remove_render_proxy(skin_entities_ptr[i]);
			}
		}

		if (_root_entities.size != 0)
			aux.free(_root_entities);

		if (_skin_entities.size != 0)
			aux.free(_skin_entities);

		_root_entities_count = 0;
		_skin_entities_count = 0;
		_root_entities		 = {};
		_skin_entities		 = {};
	}

	void trait_model_instance::instantiate_model_to_world(world& w, resource_handle model_handle)
	{
		_target_model = model_handle;

		entity_manager&	   em	   = w.get_entity_manager();
		trait_manager&	   tm	   = w.get_trait_manager();
		resource_manager&  res	   = w.get_resource_manager();
		chunk_allocator32& tm_aux  = tm.get_aux();
		chunk_allocator32& res_aux = res.get_aux();

		model&				 mdl		  = res.get_resource<model>(model_handle);
		const chunk_handle32 meshes		  = mdl.get_created_meshes();
		const chunk_handle32 nodes		  = mdl.get_created_nodes();
		const uint16		 meshes_count = mdl.get_mesh_count();
		const uint16		 nodes_count  = mdl.get_node_count();

		if (nodes_count == 0 || meshes_count == 0)
			return;

		model_node*		 ptr_nodes		   = res_aux.get<model_node>(nodes);
		resource_handle* ptr_meshes_handle = res_aux.get<resource_handle>(meshes);

		// -----------------------------------------------------------------------------
		// destroy all entities spawned for this previously.
		// -----------------------------------------------------------------------------

		if (_root_entities_count != 0)
		{
			world_handle* root_entities_ptr = tm_aux.get<world_handle>(_root_entities);
			for (uint32 i = 0; i < _root_entities_count; i++)
				em.destroy_entity(root_entities_ptr[i]);

			if (_root_entities.size != 0)
				tm_aux.free(_root_entities);

			if (_skin_entities.size != 0)
				tm_aux.free(_skin_entities);

			_skin_entities		 = {};
			_root_entities		 = {};
			_root_entities_count = 0;
			_skin_entities_count = 0;
		}

		// -----------------------------------------------------------------------------
		// create entity per model node, store the roots seperately.
		// -----------------------------------------------------------------------------

		vector<world_handle> created_node_entities;
		vector<world_handle> root_entities;
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];

#ifndef SFG_STRIP_DEBUG_NAMES
			const char* name = reinterpret_cast<const char*>(res_aux.get(node.get_name().head));
#else
			const char* name = "model_node";
#endif

			const world_handle e = em.create_entity(name);
			created_node_entities.push_back(e);

			if (node.get_parent_index() == -1)
				root_entities.push_back(e);
		}

		// -----------------------------------------------------------------------------
		// make sure any bone entities are proxied.
		// -----------------------------------------------------------------------------

		const chunk_handle32   skins	   = mdl.get_created_skins();
		const uint16		   skins_count = mdl.get_skin_count();
		const resource_handle* skins_ptr   = res_aux.get<resource_handle>(skins);
		vector<world_handle>   skin_entities;
		for (uint16 i = 0; i < skins_count; i++)
		{
			const skin&			 sk			  = res.get_resource<skin>(skins_ptr[i]);
			const chunk_handle32 joints		  = sk.get_joints();
			const uint16		 joints_count = sk.get_joints_count();
			const skin_joint*	 joints_ptr	  = res_aux.get<skin_joint>(joints);
			for (uint16 j = 0; j < joints_count; j++)
			{
				const uint16 idx = joints_ptr[j].model_node_index;
				skin_entities.push_back(created_node_entities[idx]);
				em.add_render_proxy(created_node_entities[idx]);
			}
		}

		// -----------------------------------------------------------------------------
		// store the skin entities
		// -----------------------------------------------------------------------------

		_skin_entities_count			= static_cast<uint16>(skin_entities.size());
		_skin_entities					= tm_aux.allocate<world_handle>(_skin_entities_count);
		world_handle* skin_entities_ptr = tm_aux.get<world_handle>(_skin_entities);
		for (uint32 i = 0; i < _skin_entities_count; i++)
		{
			skin_entities_ptr[i] = skin_entities[i];
		}

		// -----------------------------------------------------------------------------
		// store the root entities
		// -----------------------------------------------------------------------------

		world_handle* root_entities_ptr = nullptr;
		_root_entities_count			= static_cast<uint32>(root_entities.size());
		_root_entities					= tm_aux.allocate<world_handle>(root_entities.size(), root_entities_ptr);
		for (uint32 i = 0; i < _root_entities_count; i++)
			root_entities_ptr[i] = root_entities[i];

		// -----------------------------------------------------------------------------
		// parent-child relationships
		// -----------------------------------------------------------------------------

		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_parent_index() != -1)
				em.add_child(created_node_entities[node.get_parent_index()], created_node_entities[i]);
		}

		// -----------------------------------------------------------------------------
		// transformations
		// -----------------------------------------------------------------------------

		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			const char* name = reinterpret_cast<const char*>(res_aux.get(node.get_name().head));

			vector3 out_pos	  = vector3::zero;
			quat	out_rot	  = quat::identity;
			vector3 out_scale = vector3::zero;
			node.get_local_matrix().decompose(out_pos, out_rot, out_scale);
			em.set_entity_position(created_node_entities[i], out_pos);
			em.set_entity_rotation(created_node_entities[i], out_rot);
			em.set_entity_scale(created_node_entities[i], out_scale);
		}

		// -----------------------------------------------------------------------------
		// add components, e.g. lights, mesh instances etc.
		// -----------------------------------------------------------------------------

		const uint16 lights_count = mdl.get_light_count();
		light_raw*	 lights_ptr	  = nullptr;
		if (lights_count != 0)
			lights_ptr = res_aux.get<light_raw>(mdl.get_created_lights());

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
					trait_light.set_values(w, lr.base_color, lr.range, lr.intensity, lr.inner_cone, lr.outer_cone);
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

			resource_handle skin_handle = {};
			const int16		skin_index	= node.get_skin_index();

			if (skin_index != -1)
			{
				const resource_handle* model_skins = res_aux.get<resource_handle>(mdl.get_created_skins());
				SFG_ASSERT(mdl.get_skin_count() != 0);
				skin_handle = model_skins[skin_index];
			}

			const resource_handle& mesh_handle	= ptr_meshes_handle[node.get_mesh_index()];
			const mesh&			   m			= res.get_resource<mesh>(mesh_handle);
			const world_handle	   trait_handle = tm.add_trait<trait_mesh_instance>(entity);
			trait_mesh_instance&   mi			= tm.get_trait<trait_mesh_instance>(trait_handle);
			mi.set_mesh(w, model_handle, mesh_handle, skin_handle, skin_index != -1 ? created_node_entities.data() : nullptr, skin_index != -1 ? static_cast<uint32>(created_node_entities.size()) : 0);
		}

		auto report_entity = [&em](world_handle e) {
			const entity_meta& meta	 = em.get_entity_meta(e);
			const vector3&	   pos	 = em.get_entity_position(e);
			const quat&		   rot	 = em.get_entity_rotation(e);
			const vector3&	   scale = em.get_entity_scale(e);
			SFG_INFO("entity: {0} - gen: {1} - index: {2}", meta.name, e.generation, e.index);
		};

		for (world_handle e : root_entities)
		{
			// report_entity(e);
		}

		for (world_handle r : root_entities)
		{
			em.add_child(_header.entity, r);
		}

		if (_instantiate_callback)
			_instantiate_callback(this, model_handle, _instantiate_user_data);
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