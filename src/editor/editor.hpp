// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/common_world.hpp"
#include "math/vector2ui16.hpp"
#include "editor_first_person_controller.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct window_event;
	class game_app;
	class trait_model_instance;

	class editor
	{
	public:
		explicit editor(game_app& game);
		~editor();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void on_init();
		void on_uninit();
		void on_tick(float dt_seconds);
		void on_post_tick(double interpolation);
		void on_render();
		bool on_window_event(const window_event& ev);

	private:
		void		create_default_camera();
		void		create_demo_content();
		void		destroy_demo_content();
		void		create_demo_model();
		void		destroy_demo_model();
		static void on_model_instance_instantiate(trait_model_instance* t, resource_handle model, void* ud);

	private:
		game_app&					   _game;
		world_handle				   _camera_entity	= {};
		world_handle				   _camera_trait	= {};
		world_handle				   _demo_model_root = {};
		world_handle				   _ambient_entity	= {};
		world_handle				   _ambient_trait	= {};
		editor_first_person_controller _camera_controller;

		world_handle _gizmo_entity = {};
	};
}
