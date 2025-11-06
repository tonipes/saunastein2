// Copyright (c) 2025 Inan Evin

#include "editor_panels.hpp"

#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/common_world.hpp"

// vekt usage to build widgets
#include "math/vector4.hpp"
#include "math/vector2.hpp"

#include "gui/vekt.hpp"

namespace SFG
{
	void editor_panels::init()
	{
	}
	void editor_panels::uninit()
	{
		_entity_names.clear();
	}

	void editor_panels::update_from_world(world& w)
	{
		_entity_names.clear();

		entity_manager& em = w.get_entity_manager();
		// Enumerate entities and collect their names
		auto entities = em.get_entities();
		for (auto it = entities->handles_begin(); it != entities->handles_end(); ++it)
		{
			const world_handle h	= *it;
			const entity_meta& meta = em.get_entity_meta(h);
			if (meta.name && meta.name[0] != '\0')
				_entity_names.emplace_back(meta.name);
		}
	}

	void editor_panels::build_ui(vekt::builder& b, const vector2ui16& screen_size, vekt::font* default_font)
	{
		return;

		// Root layout: fill screen, vertical
		vekt::pos_props& root_pos = b.widget_get_pos_props(b.get_root());
		root_pos.flags			  = vekt::pos_flags::pf_child_pos_row; // allow placing side panels in a row if needed

		// Left sidebar container
		const float sidebar_width_px = 280.0f; // fixed width sidebar
		vekt::id	sidebar			 = b.allocate();
		b.widget_add_child(b.get_root(), sidebar);
		b.widget_set_pos(sidebar, vector2(0.0f, 0.0f), vekt::helper_pos_type::absolute, vekt::helper_pos_type::absolute);
		b.widget_set_size(sidebar, vector2(sidebar_width_px, static_cast<float>(screen_size.y)), vekt::helper_size_type::absolute, vekt::helper_size_type::absolute);

		// Visual background
		vekt::widget_gfx& sidebar_gfx = b.widget_get_gfx(sidebar);
		sidebar_gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
		// a subtle dark background color (sRGB-ish converted elsewhere by pipeline)
		sidebar_gfx.color = vector4(0.09f, 0.11f, 0.13f, 1.0f);

		// Inner vertical list layout (entity names)
		vekt::pos_props& sidebar_pos	  = b.widget_get_pos_props(sidebar);
		sidebar_pos.flags				  = vekt::pos_flags::pf_child_pos_column;
		vekt::size_props& sidebar_size	  = b.widget_get_size_props(sidebar);
		sidebar_size.child_margins.left	  = 8.0f;
		sidebar_size.child_margins.right  = 8.0f;
		sidebar_size.child_margins.top	  = 8.0f;
		sidebar_size.child_margins.bottom = 8.0f;
		sidebar_size.spacing			  = 4.0f;

		// Populate text entries for each entity
		for (const SFG::string& name : _entity_names)
		{
			vekt::id text = b.allocate();
			b.widget_add_child(sidebar, text);
			// left aligned, auto height, full width (relative X fill)
			b.widget_set_pos(text, vector2(0.0f, 0.0f));
			b.widget_set_size(text, vector2(1.0f, 20.0f), vekt::helper_size_type::relative, vekt::helper_size_type::absolute);

			vekt::widget_gfx& gfx = b.widget_get_gfx(text);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = vector4(0.85f, 0.90f, 0.95f, 1.0f);

			vekt::text_props& tp = b.widget_get_text(text);
			tp.font				 = default_font;
			tp.text				 = name.c_str();
			b.widget_update_text(text);
		}
	}
}
