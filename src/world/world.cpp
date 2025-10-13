// Copyright (c) 2025 Inan Evin

#include "world.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#include "app/debug_console.hpp"

#ifdef SFG_TOOLMODE
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

/* DEBUG */
#include <algorithm>
#include <execution>
#include "gfx/renderer.hpp"
#include "resources/texture.hpp"
#include "resources/texture_raw.hpp"
#include "resources/shader.hpp"
#include "resources/material.hpp"
#include "resources/model.hpp"
#include "resources/skin.hpp"
#include "resources/mesh.hpp"
#include "resources/animation.hpp"
#include "platform/time.hpp"
#include "gfx/world/world_renderer.hpp"
#include "resources/model_raw.hpp"
#include "resources/model_node.hpp"
#include "resources/primitive.hpp"
#include "traits/trait_mesh_instance.hpp"
#include "traits/trait_camera.hpp"

namespace SFG
{

	world::world(render_event_stream& rstream) : _entity_manager(*this), _resources(*this), _render_stream(rstream)
	{
		_text_allocator.init(MAX_ENTITIES * 32);
	};

	world::~world()
	{
		_text_allocator.uninit();
	}

	void world::init()
	{
		SFG_PROG("initializing world.");

		debug_console::get()->register_console_function<int>("world_set_play", [this](int b) { _flags.set(world_flags_is_playing, b != 0); });
		_flags.set(world_flags_is_init);
		_resources.init();
		_entity_manager.init();

		debug_console::get()->register_console_function<>("world_reload", [this]() {
			world_raw	 raw = {};
			const string p	 = engine_data::get().get_working_dir() + "assets/world/demo_world.stkworld";
			raw.cook_from_file(p.c_str());
			create_from_raw(raw);
		});
	}

	void world::uninit()
	{
		_entity_manager.uninit();
		_resources.uninit();
		_text_allocator.reset();

		SFG_PROG("uninitializing world.");

		debug_console::get()->unregister_console_function("world_reload");
		debug_console::get()->unregister_console_function("world_set_play");
		_flags.remove(world_flags_is_init);
	}

	void world::load_debug()
	{
		/*
		world_handle parent = _entity_manager.create_entity();
		world_handle child	 = _entity_manager.create_entity();
		_entity_manager.add_child(parent, child);

		auto report = [&]() {
			SFG_INFO("*************** Report ***************");

			const vector3 parent_pos   = _entity_manager.get_entity_position(parent);
			const vector3 parent_rot   = quat::to_euler(_entity_manager.get_entity_rotation(parent));
			const vector3 parent_scale = _entity_manager.get_entity_scale(parent);

			const vector3 parent_pos_abs   = _entity_manager.get_entity_position_abs(parent);
			const vector3 parent_rot_abs   = quat::to_euler(_entity_manager.get_entity_rotation_abs(parent));
			const vector3 parent_scale_abs = _entity_manager.get_entity_scale_abs(parent);
			SFG_INFO("parent local -> position: x:{0} y:{1} z:{2}, rotation: x:{3} y:{4} z:{5}, scale: x:{6} y:{7} z:{8}", parent_pos.x, parent_pos.y, parent_pos.z, parent_rot.x, parent_rot.y, parent_rot.z, parent_scale.x, parent_scale.y, parent_scale.z);
			SFG_INFO("parent abs   -> position: x:{0} y:{1} z:{2}, rotation: x:{3} y:{4} z:{5}, scale: x:{6} y:{7} z:{8}",
					 parent_pos_abs.x,
					 parent_pos_abs.y,
					 parent_pos_abs.z,
					 parent_rot_abs.x,
					 parent_rot_abs.y,
					 parent_rot_abs.z,
					 parent_scale_abs.x,
					 parent_scale_abs.y,
					 parent_scale_abs.z);

			const vector3 child_pos	  = _entity_manager.get_entity_position(child);
			const vector3 child_rot	  = quat::to_euler(_entity_manager.get_entity_rotation(child));
			const vector3 child_scale = _entity_manager.get_entity_scale(child);

			const vector3 child_pos_abs	  = _entity_manager.get_entity_position_abs(child);
			const vector3 child_rot_abs	  = quat::to_euler(_entity_manager.get_entity_rotation_abs(child));
			const vector3 child_scale_abs = _entity_manager.get_entity_scale_abs(child);
			SFG_INFO("               ");
			SFG_INFO("child local -> position: x:{0} y:{1} z:{2}, rotation: x:{3} y:{4} z:{5}, scale: x:{6} y:{7} z:{8}", child_pos.x, child_pos.y, child_pos.z, child_rot.x, child_rot.y, child_rot.z, child_scale.x, child_scale.y, child_scale.z);
			SFG_INFO("child abs   -> position: x:{0} y:{1} z:{2}, rotation: x:{3} y:{4} z:{5}, scale: x:{6} y:{7} z:{8}",
					 child_pos_abs.x,
					 child_pos_abs.y,
					 child_pos_abs.z,
					 child_rot_abs.x,
					 child_rot_abs.y,
					 child_rot_abs.z,
					 child_scale_abs.x,
					 child_scale_abs.y,
					 child_scale_abs.z);
		};

		report();

		_entity_manager.set_entity_position(parent, vector3(2, 5, 2));
		_entity_manager.set_entity_position_abs(child, vector3(0, 0, 0));

		_entity_manager.set_entity_rotation(parent, quat::from_euler(15, 30, 150));
		_entity_manager.set_entity_rotation_abs(child, quat::from_euler(0, 0, 100));

		_entity_manager.set_entity_scale(parent, vector3(5, 10, 2));
		_entity_manager.set_entity_scale(child, vector3(1, 1, 1));
		report();
		return;
		*/

		vector<std::function<void()>> tasks;

		const int64 mr_begin = time::get_cpu_microseconds();

		world_raw	 raw = {};
		const string p	 = engine_data::get().get_working_dir() + "assets/world/demo_world.stkworld";
		raw.cook_from_file(p.c_str());
		create_from_raw(raw);

		const int64 mr_diff = time::get_cpu_microseconds() - mr_begin;
		SFG_INFO("Resources took: {0} ms", mr_diff / 1000);

		// Camera
		const world_handle cam_entity = _entity_manager.create_entity();
		const world_handle cam_handle = _entity_manager.add_trait<trait_camera>(cam_entity);
		trait_camera&	   trait_cam  = _entity_manager.get_trait<trait_camera>(cam_handle);
		trait_cam.set_values(*this, 0.01f, 500.0f, 90.0f);
		trait_cam.set_main(*this);
		_entity_manager.set_entity_position(cam_entity, vector3(0, 4.5f, -17.5f));

		// Model test
		const resource_handle boombox		 = _resources.get_resource_handle_by_hash<model>(TO_SIDC("assets/boombox/boombox.stkmodel"));
		const world_handle	  boombox_entity = add_model_to_world(boombox, nullptr, 0);
	}

	void world::create_from_raw(world_raw& raw)
	{
		uninit();
		init();
		_resources.load_resources(raw.resources);
	}

	void world::tick(const vector2ui16& res, float dt)
	{
		_resources.tick();
	}

	void world::post_tick(double interpolation)
	{
		_entity_manager.post_tick(interpolation);
	}

	void world::pre_render(const vector2ui16& res)
	{
	}

	world_handle world::add_model_to_world(resource_handle handle, resource_handle* materials, uint32 material_size)
	{
		world_handle	   root = _entity_manager.create_entity("root");
		model&			   mdl	= _resources.get_resource<model>(handle);
		chunk_allocator32& aux	= _resources.get_aux();

		const chunk_handle32 meshes		  = mdl.get_created_meshes();
		const chunk_handle32 nodes		  = mdl.get_created_nodes();
		const uint16		 meshes_count = mdl.get_mesh_count();
		const uint16		 nodes_count  = mdl.get_node_count();

		if (nodes_count == 0 || meshes_count == 0)
			return {};

		model_node*		 ptr_nodes		   = aux.get<model_node>(nodes);
		resource_handle* ptr_meshes_handle = aux.get<resource_handle>(meshes);

		// create nodes.
		vector<world_handle> created_node_entities;
		vector<world_handle> root_entities;
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node&		   node = ptr_nodes[i];
			const char*		   name = reinterpret_cast<const char*>(aux.get(node.get_name().head));
			const world_handle e	= _entity_manager.create_entity(name);
			created_node_entities.push_back(e);
			if (node.get_parent_index() == -1)
				root_entities.push_back(e);
		}

		// assign parent child.
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_parent_index() != -1)
				_entity_manager.add_child(created_node_entities[node.get_parent_index()], created_node_entities[i]);
		}

		// assign transform hierarchy
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node	  = ptr_nodes[i];
			vector3		out_pos	  = vector3::zero;
			quat		out_rot	  = quat::identity;
			vector3		out_scale = vector3::zero;
			node.get_local_matrix().decompose(out_pos, out_rot, out_scale);
			_entity_manager.set_entity_position(created_node_entities[i], out_pos);
			_entity_manager.set_entity_rotation(created_node_entities[i], out_rot);
			_entity_manager.set_entity_scale(created_node_entities[i], out_scale);
		}

		// add meshes.
		for (uint16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_mesh_index() == -1)
				continue;

			const resource_handle& mesh_handle	= ptr_meshes_handle[node.get_mesh_index()];
			const mesh&			   m			= _resources.get_resource<mesh>(mesh_handle);
			const world_handle	   entity		= created_node_entities[i];
			const world_handle	   trait_handle = _entity_manager.add_trait<trait_mesh_instance>(entity);
			trait_mesh_instance&   mi			= _entity_manager.get_trait<trait_mesh_instance>(trait_handle);
			mi.set_mesh(*this, handle, mesh_handle);
		}

		auto report_entity = [this](world_handle e) {
			const entity_meta& meta	 = _entity_manager.get_entity_meta(e);
			const vector3&	   pos	 = _entity_manager.get_entity_position(e);
			const quat&		   rot	 = _entity_manager.get_entity_rotation(e);
			const vector3&	   scale = _entity_manager.get_entity_scale(e);
			SFG_INFO("entity: {0}", meta.name);
			SFG_INFO("position: {0} {1} {2}", pos.x, pos.y, pos.z);
			SFG_INFO("rotation: {0} {1} {2} {3}", rot.x, rot.y, rot.z, rot.w);
			SFG_INFO("scale: {0} {1} {2}", scale.x, scale.y, scale.z);
		};

		for (world_handle e : root_entities)
		{
			SFG_INFO("reporting root...");
			report_entity(e);
			SFG_INFO("reporting children...");
			_entity_manager.visit_children(e, [&](world_handle c) { report_entity(c); });
		}

		return root;
	}

#ifdef SFG_TOOLMODE

	void world::save(const char* path)
	{
		json j;
		j["dummy"] = 2;

		std::ofstream file(path);

		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
		}

		SFG_PROG("saved world: {0}", path);
	}

	void world::load(const char* path)
	{
		if (strlen(path) == 0)
			return;

		if (_flags.is_set(world_flags_is_init))
			uninit();

		if (!file_system::exists(path))
		{
			SFG_ERR("can't load world as path don't exist! {0}", path);
			return;
		}

		std::ifstream f(path);
		json		  data	= json::parse(f);
		int			  dummy = data["dummy"];
		SFG_PROG("loaded world {0}", path);

		init();
	}
#endif

	bool world::on_window_event(const window_event& ev)
	{
		if (!_flags.is_set(world_flags_is_init))
			return false;
		return false;
	}
}