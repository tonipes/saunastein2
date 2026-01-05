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

#include "gui/gui_builder.hpp"
#include "world/world_constants.hpp"
#include "data/vector.hpp"

namespace vekt
{
	class builder;
};

namespace SFG
{
	class world;

	class editor_panel_entities
	{
	public:
		void init(vekt::builder* b);
		void uninit();
		void draw(world& w, const struct vector2ui16& window_size);

		inline void set_tree_dirty()
		{
			_tree_dirty = true;
		}

	private:
		void							rebuild_tree(class world& w);
		void							build_entity_node(class world& w, world_handle e, unsigned int depth);
		static vekt::input_event_result on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static void						on_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button);
		void							open_context_menu(float x, float y, world_handle target);
		void							close_context_menu();
		bool							is_ancestor_of(world_handle ancestor, world_handle node);

	private:
		struct entity_panel_meta
		{
			unsigned char collapsed = 0;
		};

		struct node_binding
		{
			vekt::id	 widget;
			world_handle handle;
		};

	private:
		vekt::builder* _builder		= nullptr;
		gui_builder	   _gui_builder = {};
		vekt::id	   _root		= 0;
		vekt::id	   _entity_area = 0;

		vekt::id _prop_name	  = 0;
		vekt::id _prop_handle = 0;
		vekt::id _prop_pos	  = 0;
		vekt::id _prop_rot	  = 0;
		vekt::id _prop_scale  = 0;

		vector<entity_panel_meta> _entity_meta	 = {};
		vector<vekt::id>		  _node_widgets	 = {};
		vector<node_binding>	  _node_bindings = {};

		const char* _text_icon_dd			= nullptr;
		const char* _text_icon_dd_collapsed = nullptr;

		bool _tree_dirty = true;

		// Drag & Drop
		bool		 _is_dragging = false;
		world_handle _drag_source = {};

		// Context Menu
		bool		 _menu_visible = false;
		vekt::id	 _menu_root	   = 0;
		vekt::id	 _menu_scrim   = 0;
		world_handle _menu_target  = {};
		struct menu_item
		{
			const char* label  = nullptr;
			int			action = 0; // 0=add,1=remove
			vekt::id	widget = 0;
		};
		vector<menu_item> _menu_items = {};
	};
}
