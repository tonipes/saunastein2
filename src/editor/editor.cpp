// Copyright (c) 2025 Inan Evin

#include "editor.hpp"

// app
#include "app/app.hpp"
#include "app/debug_console.hpp"

#include "platform/window.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "common/string_id.hpp"
#include "project/engine_data.hpp"
#include "input/input_mappings.hpp"

// game
#include "game/game_world_renderer.hpp"

// gfx
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

// world
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/traits/trait_camera.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "world/traits/trait_ambient.hpp"
#include "world/traits/trait_light.hpp"
#include "world/traits/trait_physics.hpp"
#include "world/traits/trait_audio.hpp"
#include "world/traits/trait_canvas.hpp"

// resources
#include "resources/model.hpp"
#include "resources/model_raw.hpp"
#include "resources/world_raw.hpp"
#include "resources/material.hpp"
#include "resources/audio.hpp"
#include "resources/font.hpp"

namespace SFG
{
	editor::editor(app& game) : _game(game)
	{
	}

	editor::~editor() = default;

	void editor::init(texture_queue* tq, const vector2ui16& size)
	{
		_camera_controller.init(_game.get_world(), _game.get_window());

#ifdef SFG_TOOLMODE
		debug_console::get()->register_console_function("start_playmode", [this]() { _game.get_world().set_playmode(play_mode::full); });
		debug_console::get()->register_console_function("stop_playmode", [this]() { _game.get_world().set_playmode(play_mode::none); });
		debug_console::get()->register_console_function("start_physics", [this]() { _game.get_world().set_playmode(play_mode::physics_only); });
		debug_console::get()->register_console_function("stop_physics", [this]() { _game.get_world().set_playmode(play_mode::none); });
#endif

		// world_raw raw = {};
		// raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		// _game.get_world().create_from_loader(raw);

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
			w.get_render_stream().add_event({.index = gizmos_handle.index, .event_type = update_model_materials}, ev);

			_gizmo_entity						   = em.create_entity("gizmos");
			world_handle		  gizmos_mi_handle = tm.add_trait<trait_model_instance>(_gizmo_entity);
			trait_model_instance& gizmos_mi		   = tm.get_trait<trait_model_instance>(gizmos_mi_handle);
			gizmos_mi.instantiate_model_to_world(w, gizmos_handle);
		}*/

		_gui_renderer.init(tq, size);
	}

	void editor::uninit()
	{
		_camera_controller.uninit();
	}

	void editor::uninit_gfx()
	{
		_gui_renderer.uninit();
	}

	void editor::tick(float dt_seconds)
	{
		_camera_controller.tick(dt_seconds);
	}

	bool editor::on_window_event(const window_event& ev)
	{
		if (_camera_controller.on_window_event(ev))
			return true;

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

		return false;
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

	void editor::create_demo_content()
	{
		world& w = _game.get_world();

		entity_manager&	  em = w.get_entity_manager();
		trait_manager&	  tm = w.get_trait_manager();
		resource_manager& rm = w.get_resource_manager();

		create_demo_model();

		_ambient_entity			 = em.create_entity("ambient");
		_ambient_trait			 = tm.add_trait<trait_ambient>(_ambient_entity);
		trait_ambient& trait_amb = tm.get_trait<trait_ambient>(_ambient_trait);
		trait_amb.set_values(w, color(0.1f, 0.1f, 0.1f));

		world_handle bb = em.find_entity("BoomBox.002");
		if (!bb.is_null())
		{
			world_handle   t   = tm.add_trait<trait_physics>(bb);
			trait_physics& phy = tm.get_trait<trait_physics>(t);
			phy.set_body_type(physics_body_type::dynamic_body);
			phy.set_shape_type(physics_shape_type::sphere);
			phy.set_height_radius(0.0f, 0.5f);
			phy.create_body(w);
			phy.add_to_simulation(w);
		}

		{
			world_handle bb = em.find_entity("Ground");
			if (!bb.is_null())
			{
				world_handle   t   = tm.add_trait<trait_physics>(bb);
				trait_physics& phy = tm.get_trait<trait_physics>(t);
				phy.set_body_type(physics_body_type::static_body);
				phy.set_shape_type(physics_shape_type::plane);
				phy.set_extent(vector3(1, 1, 1));
				phy.create_body(w);
				phy.add_to_simulation(w);
			}
		}

		{
			// world_handle sound_handle = em.create_entity("sound");
			// world_handle trait		  = tm.add_trait<trait_audio>(sound_handle);
			// trait_audio& aud		  = tm.get_trait<trait_audio>(trait);
			// aud.set_audio(w, rm.get_resource_handle_by_hash<audio>(TO_SIDC("assets/audio/metal.stkaud")));
			// aud.play(w);
		}

		{
			world_handle  canvas_handle = em.create_entity("canvas");
			world_handle  trait			= tm.add_trait<trait_canvas>(canvas_handle);
			trait_canvas& cnv			= tm.get_trait<trait_canvas>(trait);
			cnv.update_counts_and_init(w, 1024, 10);
			vekt::builder* b	= cnv.get_builder();
			vekt::id	   test = b->allocate();
			b->widget_add_child(b->get_root(), test);

			b->widget_set_pos(test, vector2(0.5, 0.5), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::start);
			b->widget_set_size(test, vector2(0.25, 0.25), vekt::helper_size_type::relative, vekt::helper_size_type::relative);

			b->widget_get_text(test).text = w.get_text_allocator().allocate("hello");
			b->widget_get_text(test).font = w.get_resource_manager().get_resource_by_hash<font>(TO_SIDC("assets/fonts/roboto.stkfont")).get_vekt_font();
			b->widget_update_text(test);
		}
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
