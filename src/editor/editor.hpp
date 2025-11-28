// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/common_world.hpp"
#include "resources/common_resources.hpp"

#include "editor_camera.hpp"
#include "editor/gfx/editor_gui_renderer.hpp"

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	class app;
	class trait_model_instance;
	class texture_queue;

	class editor
	{
	public:
		explicit editor(app& game);
		~editor();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(texture_queue* tq, const vector2ui16& size);
		void uninit();
		void uninit_gfx();
		void tick(float dt_seconds);
		bool on_window_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare_render(gfx_id cmd_buffer, uint8 frame_index);
		void render_in_swapchain(gfx_id cmd_buffer, uint8 frame_index, bump_allocator& alloc);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_camera& get_camera()
		{
			return _camera_controller;
		}

	private:
		void		create_default_camera();
		void		create_demo_content();
		void		destroy_demo_content();
		void		create_demo_model();
		void		destroy_demo_model();
		static void on_model_instance_instantiate(trait_model_instance* t, resource_handle model, void* ud);

	private:
		app&		  _app;
		world_handle  _camera_entity   = {};
		world_handle  _camera_trait	   = {};
		world_handle  _demo_model_root = {};
		world_handle  _ambient_entity  = {};
		world_handle  _ambient_trait   = {};
		editor_camera _camera_controller;
		world_handle  _gizmo_entity = {};

		// gfx
		editor_gui_renderer _gui_renderer = {};
	};
}
