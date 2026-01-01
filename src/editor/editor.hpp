/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
		void tick();
		void pre_world_tick(const vector2ui16& world_res, float delta);
		void post_world_tick(const vector2ui16& world_res, float delta);
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
