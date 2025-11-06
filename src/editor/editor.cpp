// Copyright (c) 2025 Inan Evin

#include "editor.hpp"

#include "app/game_app.hpp"

#include "platform/window.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "common/string_id.hpp"
#include "project/engine_data.hpp"
#include "input/input_mappings.hpp"

// gfx
#include "gfx/renderer.hpp"
#include "gfx/world/world_renderer.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

// world
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/traits/trait_camera.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "world/traits/trait_ambient.hpp"
#include "world/traits/trait_light.hpp"

// resources
#include "resources/model.hpp"
#include "resources/model_raw.hpp"
#include "resources/world_raw.hpp"
#include "resources/material.hpp"

namespace SFG
{
	editor::editor(game_app& game) : _game(game)
	{
	}

	editor::~editor() = default;

	void editor::init(texture_queue* tq, const vector2ui16& size)
	{
		// Panels init
		world_raw raw = {};
		raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		_game.get_world().create_from_loader(raw);

		create_default_camera();
		create_demo_content();

		/*
		{
			world&			  w	 = _game.get_world();
			entity_manager&	  em = w.get_entity_manager();
			trait_manager&	  tm = w.get_trait_manager();
			resource_manager& rm = w.get_resource_manager();

			rm.load_resources(
				{
					"assets/engine/shaders/gizmo/gizmo.stkshader",
					"assets/engine/materials/gizmo_x.stkmat",
					"assets/engine/materials/gizmo_y.stkmat",
					"assets/engine/materials/gizmo_z.stkmat",
					"assets/engine/models/gizmos/gizmos.stkmodel",
				},
				false,
				SFG_ROOT_DIRECTORY);

			// gizmo model (all).
			resource_handle gizmos_handle = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/engine/models/gizmos/gizmos.stkmodel"));

			// get gizmo materials.
			const resource_handle gizmo_mat0 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_x.stkmat"));
			const resource_handle gizmo_mat1 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_y.stkmat"));
			const resource_handle gizmo_mat2 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_z.stkmat"));

			// update model materials.
			const render_event_model_update_materials ev = {
				.materials =
					{
						gizmo_mat1.index,
						gizmo_mat2.index,
						gizmo_mat0.index,
					},
			};
			w.get_render_stream().add_event({.index = gizmos_handle.index, .event_type = render_event_update_model_materials}, ev);

			_gizmo_entity						   = em.create_entity("gizmos");
			world_handle		  gizmos_mi_handle = tm.add_trait<trait_model_instance>(_gizmo_entity);
			trait_model_instance& gizmos_mi		   = tm.get_trait<trait_model_instance>(gizmos_mi_handle);
			gizmos_mi.instantiate_model_to_world(w, gizmos_handle);
		}*/

		_gui_renderer.init(tq, size);
	}

	void editor::uninit()
	{
		destroy_demo_content();
	}

	void editor::uninit_gfx()
	{
		_gui_renderer.uninit();
	}
	void editor::tick(float dt_seconds)
	{
		if (_camera_controller.is_active())
			_camera_controller.tick(dt_seconds);
	}

	void editor::post_tick(double)
	{
	}

	bool editor::on_window_event(const window_event& ev)
	{
		if (_camera_controller.is_active())
			_camera_controller.on_window_event(ev);

		if (ev.type == window_event_type::key && ev.sub_type == window_event_sub_type::press)
		{
			if (ev.button == input_code::key_k)
			{
				create_demo_model();
				return true;
			}
			else if (ev.button == input_code::key_l)
			{
				destroy_demo_model();
				return true;
			}
		}

		if (ev.type == window_event_type::mouse && ev.sub_type == window_event_sub_type::press)
		{
			if (ev.button == input_code::mouse_0 && ev.value.x > 0 && ev.value.y > 0)
			{
				// const uint32	   id	= _game.get_renderer()->get_world_renderer()->get_render_pass_object_id().read_location(ev.value.x, ev.value.y, 0);
				// const entity_meta& meta = _game.get_world().get_entity_manager().get_entity_meta({.generation = 2, .index = id});
				// _game.get_world().get_entity_manager().set_entity_position(_gizmo_entity, _game.get_world().get_entity_manager().get_entity_position_abs({.generation = 2, .index = id}));
				// _game.get_world().get_entity_manager().teleport_entity(_gizmo_entity);
				// _game.get_renderer()->get_world_renderer()->get_render_pass_selection_outline().set_selected_entity_id(id == 0 ? NULL_WORLD_ID : id);
				// SFG_WARN("Pressed on object {0} - name: {1}", id, meta.name);
			}
		}

		return true;
	}

	void editor::prepare_render(gfx_id cmd_buffer, uint8 frame_index)
	{
		_gui_renderer.prepare(cmd_buffer, frame_index);
	}

	void editor::render_in_swapchain(gfx_id cmd_buffer, uint8 frame_index, bump_allocator& alloc)
	{
		_gui_renderer.render_in_swapchain(cmd_buffer, frame_index, alloc);
	}

	void editor::resize(const vector2ui16& size)
	{
		_gui_renderer.resize(size);
	}

	void editor::create_default_camera()
	{

		world&			w  = _game.get_world();
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();
		_camera_entity	   = em.create_entity("editor_camera");
		_camera_trait	   = tm.add_trait<trait_camera>(_camera_entity);

		trait_camera& cam_trait = tm.get_trait<trait_camera>(_camera_trait);
		cam_trait.set_values(w, 0.01f, 250.0f, 60.0f, {0.01f, 0.04f, 0.125f, 0.25f, 0.5f});
		cam_trait.set_main(w);

		em.set_entity_position(_camera_entity, vector3(0.0f, 0, 5));
		em.set_entity_rotation(_camera_entity, quat::identity);

		window* main_window = _game.get_main_window();
		_camera_controller.init(w, _camera_entity, main_window);
	}

	void editor::create_demo_content()
	{
		world& w = _game.get_world();

		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		create_demo_model();

		_ambient_entity			 = em.create_entity("ambient");
		_ambient_trait			 = tm.add_trait<trait_ambient>(_ambient_entity);
		trait_ambient& trait_amb = tm.get_trait<trait_ambient>(_ambient_trait);
		trait_amb.set_values(w, color(0.1f, 0.1f, 0.1f));
	}

	void editor::destroy_demo_content()
	{
		world&			w  = _game.get_world();
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		destroy_demo_model();

		if (!_ambient_entity.is_null())
		{
			tm.remove_trait<trait_ambient>(_ambient_entity, _ambient_trait);
			em.destroy_entity(_ambient_entity);
			_ambient_entity = {};
			_ambient_trait	= {};
		}

		_camera_controller.uninit();

		if (!_camera_trait.is_null())
		{
			tm.remove_trait<trait_camera>(_camera_entity, _camera_trait);
			_camera_trait = {};
		}

		if (!_camera_entity.is_null())
		{
			em.destroy_entity(_camera_entity);
			_camera_entity = {};
		}
	}

	void editor::create_demo_model()
	{
		world&			  w	 = _game.get_world();
		entity_manager&	  em = w.get_entity_manager();
		trait_manager&	  tm = w.get_trait_manager();
		resource_manager& rm = w.get_resource_manager();

		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/test_scene/test_scene.stkmodel"));
		if (!rm.is_valid<model>(boombox))
			return;

		_demo_model_root = em.create_entity("boombox_root");
		em.set_entity_position(_demo_model_root, vector3::zero);
		em.set_entity_rotation(_demo_model_root, quat::identity);

		const world_handle	  model_inst_handle = tm.add_trait<trait_model_instance>(_demo_model_root);
		trait_model_instance& mi				= tm.get_trait<trait_model_instance>(model_inst_handle);
		mi.set_model(boombox);
		mi.set_instantiate_callback(on_model_instance_instantiate, this);
		mi.instantiate_model_to_world(w, boombox);
	}

	void editor::destroy_demo_model()
	{
		world&			w  = _game.get_world();
		entity_manager& em = w.get_entity_manager();
		em.destroy_entity(_demo_model_root);
	}

	void editor::on_model_instance_instantiate(trait_model_instance* t, resource_handle model, void* ud)
	{
		world&			w  = (static_cast<editor*>(ud)->_game).get_world();
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		const world_handle sun_handle = em.find_entity("Sun");
		if (!sun_handle.is_null())
		{
			const world_handle dir_light_handle = em.get_entity_trait<trait_dir_light>(sun_handle);
			if (!dir_light_handle.is_null())
			{
				trait_dir_light& t = tm.get_trait<trait_dir_light>(dir_light_handle);
				t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
			}
		}

		const world_handle spot_handle = em.find_entity("Spot");
		if (!spot_handle.is_null())
		{
			const world_handle handle = em.get_entity_trait<trait_spot_light>(spot_handle);
			if (!handle.is_null())
			{
				trait_spot_light& t = tm.get_trait<trait_spot_light>(handle);
				t.set_values(w, t.get_color(), 50.0f, t.get_intensity(), t.get_inner_cone(), t.get_outer_cone());
				t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
			}
		}

		const world_handle point_handle = em.find_entity("P1");
		if (!point_handle.is_null())
		{
			const world_handle handle = em.get_entity_trait<trait_point_light>(point_handle);
			if (!handle.is_null())
			{
				trait_point_light& t = tm.get_trait<trait_point_light>(handle);
				t.set_values(w, t.get_color(), 50.0f, t.get_intensity());
				t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
			}
		}

		{

			const world_handle point_handle = em.find_entity("RedLight");
			if (!point_handle.is_null())
			{
				const world_handle handle = em.get_entity_trait<trait_point_light>(point_handle);
				if (!handle.is_null())
				{
					trait_point_light& t = tm.get_trait<trait_point_light>(handle);
					t.set_values(w, t.get_color(), 50.0f, t.get_intensity());
					t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
				}
			}
		}
	}
}
