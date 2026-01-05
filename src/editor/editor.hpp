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
#include "memory/bump_text_allocator.hpp"
#include "memory/text_allocator.hpp"

// gui
#include "gui/vekt_defines.hpp"
#include "editor/gui/editor_gui_controller.hpp"

namespace vekt
{
	struct font;
	class font_manager;
	class builder;
}

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	class app;
	class comp_model_instance;
	class texture_queue;
	class proxy_manager;

	class editor
	{
	public:
		struct render_params
		{
			proxy_manager&	pm;
			gfx_id			cmd_buffer;
			uint8			frame_index;
			bump_allocator& alloc;
			vector2ui16		size;
			gfx_id			global_layout;
			gfx_id			global_group;
			gpu_index		world_rt_index;
		};

		explicit editor(app& game);
		~editor();

		static editor& get()
		{
			return *s_instance;
		}
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void			   init();
		void			   uninit();
		void			   tick();
		void			   render(const render_params& p);
		void			   pre_world_tick(float delta);
		void			   post_world_tick(float delta);
		bool			   on_window_event(const window_event& ev);
		void			   resize(const vector2ui16& size);
		void			   on_file_dropped(const char* path);
		const vector2ui16& get_game_resolution() const;

		// -----------------------------------------------------------------------------
		// level
		// -----------------------------------------------------------------------------

		void load_level_prompt();
		void load_level(const char* relative_path);
		void save_lavel();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_camera& get_camera()
		{
			return _camera_controller;
		}

		inline editor_renderer& get_renderer()
		{
			return _renderer;
		}

		inline editor_gui_controller& get_gui_controller()
		{
			return _gui_controller;
		}

		inline gpu_index get_render_output(uint8 frame) const
		{
			return _renderer.get_output_gpu_index(frame);
		}

		inline app& get_app()
		{
			return _app;
		}

		inline bump_text_allocator& get_bump_text_allocator()
		{
			return _bump_text_allocator;
		}

		inline text_allocator& get_text_allocator()
		{
			return _text_allocator;
		}

	private:
		app&		 _app;
		world_handle _camera_entity	  = {};
		world_handle _camera_trait	  = {};
		world_handle _demo_model_root = {};
		world_handle _ambient_entity  = {};
		world_handle _ambient_trait	  = {};
		world_handle _gizmo_entity	  = {};

		editor_camera _camera_controller;

		// gfx
		editor_renderer _renderer = {};

		// gui
		bump_text_allocator	  _bump_text_allocator = {};
		text_allocator		  _text_allocator	   = {};
		editor_gui_controller _gui_controller	   = {};

		// vekt
		vekt::builder*		_builder	  = nullptr;
		vekt::font_manager* _font_manager = nullptr;
		vekt::font*			_font_main	  = nullptr;
		vekt::font*			_font_title	  = nullptr;
		vekt::font*			_font_icons	  = nullptr;

		static editor* s_instance;
	};
}
