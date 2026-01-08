/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

See root license for details.
*/

#pragma once

#include "world/world_constants.hpp"
#include "math/vector2ui16.hpp"
#include "gui/vekt_defines.hpp"
#include "platform/window_common.hpp"

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

		bool on_mouse_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------

		vekt::id begin_context_menu(float abs_x, float abs_y);
		vekt::id add_context_menu_item(const char* label);
		void	 end_context_menu();

		void enable_payload(const char* text);
		void disable_payload();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_panel_entities* get_entities() const
		{
			return _panel_entities;
		}

		inline vekt::id get_root() const
		{
			return _ctx_root;
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
		uint64					   _ctx_frame		   = 0;

		vekt::id _ctx_root		 = NULL_WIDGET_ID;
		vekt::id _ctx_active	 = NULL_WIDGET_ID;
		vekt::id _payload		 = NULL_WIDGET_ID;
		vekt::id _payload_text	 = NULL_WIDGET_ID;
		uint8	 _payload_active = 0;
	};
}
