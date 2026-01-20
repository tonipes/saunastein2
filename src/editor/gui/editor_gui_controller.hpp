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

#include "world/world_constants.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector2.hpp"
#include "gui/vekt_defines.hpp"
#include "platform/window_common.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class world;
	class editor_panel_stats;
	class editor_panel_entities;
	class editor_panel_properties;
	class editor_gui_world_overlays;
	class editor_panel_world;

	class editor_gui_controller
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b);
		void uninit();
		void tick(world& w, const vector2ui16& window_size);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool	on_mouse_event(const window_event& ev);
		bool	on_key_event(const window_event& ev);
		vector2 get_world_size();
		void	on_exited_playmode();

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------

		vekt::id begin_context_menu(float abs_x, float abs_y);
		vekt::id add_context_menu_item(const char* label);
		vekt::id add_context_menu_item_toggle(const char* label, bool is_toggled);
		void	 end_context_menu();
		void	 enable_payload(const char* text);
		void	 disable_payload();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_panel_entities* get_entities() const
		{
			return _panel_entities;
		}

		inline editor_panel_stats* get_stats() const
		{
			return _panel_stats;
		}

		inline vekt::id get_root() const
		{
			return _ctx_root;
		}

	private:
		static void on_context_item_hover_begin(vekt::builder* b, vekt::id widget);
		static void on_context_item_hover_end(vekt::builder* b, vekt::id widget);
		static void on_separator_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button);
		static void on_separator_hover_begin(vekt::builder* b, vekt::id widget);
		static void on_separator_hover_end(vekt::builder* b, vekt::id widget);

	private:
		vekt::builder*		   _builder		   = nullptr;
		editor_panel_stats*	   _panel_stats	   = nullptr;
		editor_panel_entities* _panel_entities = nullptr;
		editor_panel_world*	   _panel_world	   = nullptr;
		uint64				   _ctx_frame	   = 0;

		vekt::id _ctx_root		 = NULL_WIDGET_ID;
		vekt::id _ctx_active	 = NULL_WIDGET_ID;
		vekt::id _payload		 = NULL_WIDGET_ID;
		vekt::id _payload_text	 = NULL_WIDGET_ID;
		uint8	 _payload_active = 0;

		// Layout root (row) and children
		vekt::id _layout_root	   = NULL_WIDGET_ID;
		vekt::id _layout_separator = NULL_WIDGET_ID;
		float	 _split_px		   = 320.0f;
		float	 _split_ratio	   = 0.25f;
	};
}
