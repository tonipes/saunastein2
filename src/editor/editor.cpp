// Copyright (c) 2025 Inan Evin

#include "editor.hpp"

#include "app/game_app.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/traits/trait_camera.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "world/traits/trait_ambient.hpp"
#include "world/traits/trait_light.hpp"
#include "resources/model.hpp"
#include "platform/window.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "common/string_id.hpp"

#include "resources/world_raw.hpp"
#include "project/engine_data.hpp"

namespace SFG
{
	editor::editor(game_app& game) : _game(game)
	{
	}

	editor::~editor() = default;

	void editor::on_init()
	{

		world_raw	 raw = {};
		const string p	 = engine_data::get().get_working_dir() + "assets/world/demo_world.stkworld";
		raw.load_from_file(p.c_str());
		_game.get_world()->create_from_loader(raw);

		create_default_camera();
		create_demo_content();

		/*
		debug_console::get()->register_console_function<int, int>("vsync", [this](int v, int tearing) {
			uint8 flags = {};

			if (v == 1)
				flags |= swapchain_flags::sf_vsync_every_v_blank;
			else if (v == 2)
				flags |= swapchain_flags::sf_vsync_every_2v_blank;
			if (tearing == 1)
				flags |= swapchain_flags::sf_allow_tearing;

			SFG_INFO("Setting VSync: {0}, Tearing: {1}", v, tearing);

			join_render();
			_renderer->on_swapchain_flags(flags);
			kick_off_render();
		});

		*/
	}

	void editor::on_uninit()
	{
		destroy_demo_content();
	}

	void editor::on_tick(float dt_seconds)
	{
		if (_camera_controller.is_active())
			_camera_controller.tick(dt_seconds);
	}

	void editor::on_post_tick(double)
	{
	}

	void editor::on_render()
	{
	}

	bool editor::on_window_event(const window_event& ev)
	{

		if (_camera_controller.is_active())
			_camera_controller.on_window_event(ev);

		return true;
	}

	void editor::create_default_camera()
	{
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&			w  = *world_ptr;
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
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&				  w		  = *world_ptr;
		resource_manager&	  rm	  = w.get_resource_manager();
		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/test_scene/test_scene.stkmodel"));

		if (!rm.is_valid<model>(boombox))
			return;
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		_demo_model_root = em.create_entity("boombox_root");
		em.set_entity_position(_demo_model_root, vector3::zero);
		em.set_entity_rotation(_demo_model_root, quat::identity);

		const world_handle	  model_inst_handle = tm.add_trait<trait_model_instance>(_demo_model_root);
		trait_model_instance& mi				= tm.get_trait<trait_model_instance>(model_inst_handle);
		mi.set_model(boombox);
		mi.instantiate_model_to_world(w, boombox);

		_ambient_entity			 = em.create_entity("ambient");
		_ambient_trait			 = tm.add_trait<trait_ambient>(_ambient_entity);
		trait_ambient& trait_amb = tm.get_trait<trait_ambient>(_ambient_trait);
		trait_amb.set_values(w, color(0.1f, 0.1f, 0.1f));

		//const world_handle sun_handle = em.find_entity("Sun");
		//if (!sun_handle.is_null())
		//{
		//	const world_handle dir_light_handle = em.get_entity_trait<trait_dir_light>(sun_handle);
		//	if (!dir_light_handle.is_null())
		//	{
		//		trait_dir_light& t = tm.get_trait<trait_dir_light>(dir_light_handle);
		//		t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
		//	}
		//}
		//
		//const world_handle spot_handle = em.find_entity("Spot");
		//if (!spot_handle.is_null())
		//{
		//	const world_handle handle = em.get_entity_trait<trait_spot_light>(spot_handle);
		//	if (!handle.is_null())
		//	{
		//		trait_spot_light& t = tm.get_trait<trait_spot_light>(handle);
		//		t.set_values(w, t.get_color(), 50.0f, t.get_intensity(), t.get_inner_cone(), t.get_outer_cone());
		//		t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
		//	}
		//}

		const world_handle point_handle = em.find_entity("P1");
		if (!point_handle.is_null())
		{
			const world_handle handle = em.get_entity_trait<trait_point_light>(point_handle);
			if (!handle.is_null())
			{
				trait_point_light& t = tm.get_trait<trait_point_light>(handle);
				// t.set_values(w, t.get_color(), 50.0f, t.get_intensity());
				t.set_shadow_values(w, 1, vector2ui16(1024, 1024));
			}
		}
	}

	void editor::destroy_demo_content()
	{
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&			w  = *world_ptr;
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

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

		if (!_demo_model_root.is_null())
		{
			em.destroy_entity(_demo_model_root);
			_demo_model_root = {};
		}
	}
}
