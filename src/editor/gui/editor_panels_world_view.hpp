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

#include "gui/vekt_defines.hpp"
#include "editor/gui/editor_gui_builder.hpp"
#include "editor/gui/editor_gui_user_data.hpp"

namespace SFG
{
	struct vector2ui16;

	class editor_panels_world_view
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b);
		void uninit();
		void draw(const vector2ui16& window_size);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool consume_committed_size(vector2ui16& out_size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline vekt::id get_root() const
		{
			return _root;
		}

		static vekt::input_event_result on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);

	private:
		gui_builder	   _gui_builder = {};
		vekt::builder* _builder		= nullptr;
		vekt::id	   _root		= NULL_WIDGET_ID;

		editor_gui_user_data _user_data = {};

		// Icon column and buttons
		vekt::id _icon_column = NULL_WIDGET_ID;
		vekt::id _btn_view	  = NULL_WIDGET_ID;
		vekt::id _btn_menu	  = NULL_WIDGET_ID;
		vekt::id _btn_stats	  = NULL_WIDGET_ID;
		vekt::id _btn_play	  = NULL_WIDGET_ID;

		vekt::id _ctx_world_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_colors_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_normals_rt  = NULL_WIDGET_ID;
		vekt::id _ctx_orm_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_emissive_rt = NULL_WIDGET_ID;
		vekt::id _ctx_depth_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_lighting_rt = NULL_WIDGET_ID;
		vekt::id _ctx_bloom_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_ssao_rt	  = NULL_WIDGET_ID;

		// Context menu items
		vekt::id _ctx_new_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_open_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_package_project = NULL_WIDGET_ID;
		vekt::id _ctx_new_world		  = NULL_WIDGET_ID;
		vekt::id _ctx_save_world	  = NULL_WIDGET_ID;
		vekt::id _ctx_open_world	  = NULL_WIDGET_ID;

		// Stats area and texts
		vekt::id _stats_area		= NULL_WIDGET_ID;
		vekt::id _wv_game_res		= NULL_WIDGET_ID;
		vekt::id _wv_window_res		= NULL_WIDGET_ID;
		vekt::id _wv_fps			= NULL_WIDGET_ID;
		vekt::id _wv_main			= NULL_WIDGET_ID;
		vekt::id _wv_render			= NULL_WIDGET_ID;
		vekt::id _wv_ram			= NULL_WIDGET_ID;
		vekt::id _wv_vram			= NULL_WIDGET_ID;
		vekt::id _wv_vram_txt		= NULL_WIDGET_ID;
		vekt::id _wv_vram_res		= NULL_WIDGET_ID;
		vekt::id _wv_draw_calls		= NULL_WIDGET_ID;
		vekt::id _wv_loaded_project = NULL_WIDGET_ID;
		vekt::id _wv_loaded_level	= NULL_WIDGET_ID;
	};
}
