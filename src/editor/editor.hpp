// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/common_world.hpp"
#include "resources/common_resources.hpp"

#include "editor_camera.hpp"
#include "editor/gfx/editor_renderer.hpp"

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	class app;
	class comp_model_instance;
	class texture_queue;

	class editor
	{
	public:
		explicit editor(app& game);
		~editor();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick(float dt_seconds);
		bool on_window_event(const window_event& ev);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_camera& get_camera()
		{
			return _camera_controller;
		}

		inline editor_renderer& get_renderer()
		{
			return _gui_renderer;
		}

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
		editor_renderer _gui_renderer = {};
	};
}
