/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

See root license for details.
*/

#pragma once

#include "world/world_constants.hpp"
#include "math/vector2ui16.hpp"
#include "gui/vekt_defines.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class world;
	class editor_panel_controls;
	class editor_panel_entities;
	class editor_panel_properties;
	class editor_gui_world_overlays;
	class editor_panels_docking;
	class editor_panels_world_view;

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

		void set_entities_tree_dirty();

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------

		vekt::id begin_context_menu(float abs_x, float abs_y);
		vekt::id add_context_menu_item(const char* label);
		void	 end_context_menu();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_selected_entity(world_handle e)
		{
			_selected_entity = e;
		}
		inline world_handle get_selected_entity() const
		{
			return _selected_entity;
		}

	private:
		static void on_context_item_hover_begin(vekt::builder* b, vekt::id widget);
		static void on_context_item_hover_end(vekt::builder* b, vekt::id widget);

	private:
		vekt::builder*			   _builder			   = nullptr;
		editor_panel_controls*	   _panel_controls	   = nullptr;
		editor_panel_entities*	   _panel_entities	   = nullptr;
		editor_panel_properties*   _panel_properties   = nullptr;
		editor_gui_world_overlays* _gui_world_overlays = nullptr;
		editor_panels_docking*	   _panels_docking	   = nullptr;
		editor_panels_world_view*  _panel_world_view   = nullptr;
		world_handle			   _selected_entity	   = {};
		vekt::id				   _ctx_root		   = 0;
	};
}
