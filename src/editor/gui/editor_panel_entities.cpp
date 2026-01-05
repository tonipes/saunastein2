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

#include "editor_panel_entities.hpp"
#include "editor/editor_theme.hpp"

#include "io/log.hpp"

#include "world/world.hpp"
#include "world/entity_manager.hpp"

#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "gui/vekt.hpp"
#include "memory/bump_text_allocator.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "gui/icon_defs.hpp"
#include "input/input_mappings.hpp"
#include "app/app.hpp"

namespace SFG
{

	void editor_panel_entities::init(vekt::builder* b)
	{
		_builder = b;

		// gui_builder now uses editor_theme directly for styles and fonts.
		_gui_builder.init(b, &editor::get().get_text_allocator());

		_gui_builder.callbacks.user_data = this;
		_root							 = _gui_builder.get_root();

		_gui_builder.add_title("entities");
		_entity_area													 = _gui_builder.begin_area(true);
		_builder->widget_get_size_props(_entity_area).child_margins.left = 0.0f;
		_gui_builder.end_area();

		// Properties section
		_gui_builder.add_title("properties");
		_gui_builder.begin_area(true);
		_prop_name	 = _gui_builder.add_property_row_label("name:", "").second;
		_prop_handle = _gui_builder.add_property_row_label("handle:", "").second;
		_prop_pos	 = _gui_builder.add_property_row_label("position:", "").second;
		_prop_rot	 = _gui_builder.add_property_row_label("rotation:", "").second;
		_prop_scale	 = _gui_builder.add_property_row_label("scale:", "").second;
		_gui_builder.end_area();

		_builder->widget_add_child(_builder->get_root(), _root);

		// Reserve per-entity meta storage
		_entity_meta.resize(MAX_ENTITIES);
		_node_bindings.reserve(512);
		_node_widgets.reserve(512);

		_text_icon_dd_collapsed = editor::get().get_text_allocator().allocate(ICON_DD_RIGHT);
		_text_icon_dd			= editor::get().get_text_allocator().allocate(ICON_DD_DOWN);
	}

	void editor_panel_entities::uninit()
	{
		editor::get().get_text_allocator().deallocate(_text_icon_dd);
		_gui_builder.uninit();
	}

	void editor_panel_entities::draw(world& w, const vector2ui16& window_size)
	{
		_builder->widget_set_pos_abs(_root, vector2(0, 50));
		_builder->widget_set_size_abs(_root, vector2(window_size.x * 0.2f, window_size.y - 50));

		if (_tree_dirty)
		{
			rebuild_tree(w);
			_tree_dirty = false;
		}

		const world_handle	 selected = editor::get().get_gui_controller().get_selected_entity();
		bump_text_allocator& alloc	  = editor::get().get_bump_text_allocator();
		entity_manager&		 em		  = w.get_entity_manager();

		const char* name_txt					   = selected.is_null() ? "-" : em.get_entity_meta(selected).name;
		_builder->widget_get_text(_prop_name).text = name_txt;
		_builder->widget_update_text(_prop_name);

		// Handle
		{
			const char* handle_txt = alloc.allocate_reserve(32);
			if (selected.is_null())
			{
				alloc.append("-");
			}
			else
			{
				alloc.append("[");
				alloc.append(static_cast<uint32>(selected.generation));
				alloc.append(", ");
				alloc.append(static_cast<uint32>(selected.index));
				alloc.append("]");
			}

			_builder->widget_get_text(_prop_handle).text = handle_txt;
			_builder->widget_update_text(_prop_handle);
		}

		// Transforms
		auto vec3_to_txt = [&alloc](const vector3& v) -> const char* {
			const char* t = alloc.allocate_reserve(64);
			alloc.appendf("%.3f, %.3f, %.3f", v.x, v.y, v.z);
			return t;
		};

		if (selected.is_null())
		{
			const char* dash							= "-";
			_builder->widget_get_text(_prop_pos).text	= dash;
			_builder->widget_get_text(_prop_rot).text	= dash;
			_builder->widget_get_text(_prop_scale).text = dash;
		}
		else
		{
			const vector3 pos = em.get_entity_position(selected);
			const vector3 scl = em.get_entity_scale(selected);
			const quat&	  rot = em.get_entity_rotation(selected);

			const char* pos_txt = vec3_to_txt(pos);
			const char* rot_txt = alloc.allocate_reserve(64);
			alloc.appendf("%.3f, %.3f, %.3f, %.3f", rot.x, rot.y, rot.z, rot.w);
			const char* scl_txt = vec3_to_txt(scl);

			_builder->widget_get_text(_prop_pos).text	= pos_txt;
			_builder->widget_get_text(_prop_rot).text	= rot_txt;
			_builder->widget_get_text(_prop_scale).text = scl_txt;
		}

		_builder->widget_update_text(_prop_pos);
		_builder->widget_update_text(_prop_rot);
		_builder->widget_update_text(_prop_scale);
	}

	void editor_panel_entities::rebuild_tree(world& w)
	{
	
		// Remove old nodes
		for (vekt::id nid : _node_widgets)
		{
			_builder->deallocate(nid);
		}
		_node_widgets.resize(0);
		_node_bindings.resize(0);

		entity_manager& em = w.get_entity_manager();
		// Build from roots
		for (auto it = em.get_entities()->handles_begin(); it != em.get_entities()->handles_end(); ++it)
		{
			world_handle		 h	 = *it;
			const entity_family& fam = em.get_entity_family(h);
			if (fam.parent.is_null())
			{
				build_entity_node(w, h, 0);
			}
		}
	}

	void editor_panel_entities::build_entity_node(world& w, world_handle e, unsigned int depth)
	{
		entity_manager& em			 = w.get_entity_manager();
		const bool		is_collapsed = _entity_meta[e.index].collapsed == 1;
		const bool		selected	 = editor::get().get_gui_controller().get_selected_entity() == e;

		static uint8 color_alt = 0;
		color_alt			   = (color_alt + 1) % 2;

		// Row container
		const vekt::id row = _builder->allocate();
		{
			_builder->widget_add_child(_entity_area, row);

			vekt::pos_props& pp = _builder->widget_get_pos_props(row);
			pp.flags			= vekt::pos_flags::pf_x_relative;
			pp.pos.x			= 0.0f;

			vekt::size_props& sz   = _builder->widget_get_size_props(row);
			sz.flags			   = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_abs;
			sz.size.x			   = 1.0f;
			sz.size.y			   = editor_theme::get().row_height;
			sz.child_margins	   = {0.0f, 0.0f, 0.0f, editor_theme::get().inner_margin};
			sz.child_margins.left  = editor_theme::get().outer_margin;
			sz.child_margins.right = editor_theme::get().outer_margin;
		}

		// Row container
		const vekt::id row_inner = _builder->allocate();
		{
			_builder->widget_add_child(row, row_inner);

			vekt::pos_props& pp = _builder->widget_get_pos_props(row_inner);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_child_pos_row;
			pp.pos.x			= 0.0f;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(row_inner);
			sz.flags			 = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_relative;
			sz.size.x			 = 1.0f;
			sz.size.y			 = 1.0f;
			sz.child_margins	 = {0.0f, 0.0f, (depth + 1) * editor_theme::get().outer_margin, editor_theme::get().inner_margin};
			sz.spacing			 = editor_theme::get().item_spacing;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(row_inner);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_rounding;
			gfx.color			  = selected ? editor_theme::get().col_accent_second_dim : (vector4());

			vekt::rounding_props& rp = _builder->widget_get_rounding(row_inner);
			rp.rounding				 = editor_theme::get().frame_rounding;
			rp.segments				 = 12;

			vekt::hover_callback& hb = _builder->widget_get_hover_callbacks(row_inner);
			hb.receive_mouse		 = 17;

			vekt::mouse_callback& mc = _builder->widget_get_mouse_callbacks(row_inner);
			mc.on_mouse				 = on_mouse;
			mc.on_drag				 = on_drag;

			_builder->widget_get_user_data(row_inner).ptr = this;
		}

		const vekt::id icon = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, icon);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(icon);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(icon);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(icon);
			tp.text				 = is_collapsed ? _text_icon_dd_collapsed : _text_icon_dd;
			tp.font				 = editor_theme::get().font_icons;
			tp.scale			 = 0.7f;
			_builder->widget_update_text(icon);
		}

		const vekt::id txt = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, txt);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.text				 = em.get_entity_meta(e).name;
			tp.font				 = editor_theme::get().font_default;
			_builder->widget_update_text(txt);
		}

		_node_widgets.push_back(row);
		_node_bindings.push_back({row_inner, e});

		const entity_family& fam = em.get_entity_family(e);
		world_handle		 ch	 = fam.first_child;

		if (ch.is_null())
		{
			_builder->widget_set_visible(icon, false);
		}
		if (!is_collapsed)
		{
			while (!ch.is_null())
			{
				build_entity_node(w, ch, depth + 1);
				ch = em.get_entity_family(ch).next_sibling;
			}
		}
	}

	vekt::input_event_result editor_panel_entities::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		// ensure hover
		const vekt::hover_callback& hb = b->widget_get_hover_callbacks(widget);
		if (!hb.is_hovered)
			return vekt::input_event_result::not_handled;

		// find entity ret if empty.
		world_handle clicked = {};
		for (const node_binding& nb : self->_node_bindings)
		{
			if (nb.widget == widget)
			{
				clicked = nb.handle;
				break;
			}
		}
		if (clicked.is_null())
			return vekt::input_event_result::not_handled;

		// context
		if (ev.button == static_cast<uint16>(input_code::mouse_1) && ev.type == vekt::input_event_type::pressed)
		{
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
			editor::get().get_gui_controller().add_context_menu_item("test");
			editor::get().get_gui_controller().add_context_menu_item("test2");
			editor::get().get_gui_controller().end_context_menu();
			// self->open_context_menu(ev.position.x, ev.position.y, clicked);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::repeated)
		{
			self->_entity_meta[clicked.index].collapsed ^= 1;
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		if (ev.button == static_cast<uint16>(input_code::mouse_0) && ev.type == vekt::input_event_type::pressed)
		{
			// deselect previous
			const world_handle existing = editor::get().get_gui_controller().get_selected_entity();
			if (!existing.is_null())
			{
				auto it = std::find_if(self->_node_bindings.begin(), self->_node_bindings.end(), [existing](const node_binding& nb) -> bool { return existing == nb.handle; });
				if (it != self->_node_bindings.end())
				{
					vekt::widget_gfx& gfx = b->widget_get_gfx(it->widget);
					gfx.color			  = {0, 0, 0, 0};
				}
			}

			// select new
			editor::get().get_gui_controller().set_selected_entity(clicked);
			auto it = std::find_if(self->_node_bindings.begin(), self->_node_bindings.end(), [clicked](const node_binding& nb) -> bool { return clicked == nb.handle; });
			if (it != self->_node_bindings.end())
			{
				vekt::widget_gfx& gfx = b->widget_get_gfx(it->widget);
					gfx.color			  = editor_theme::get().col_accent_second_dim;
			}

			self->_is_dragging = true;
			self->_drag_source = clicked;
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::released && ev.button == static_cast<uint16>(input_code::mouse_0))
		{
			if (self->_is_dragging && !self->_drag_source.is_null())
			{
				world_handle src = self->_drag_source;
				world_handle dst = clicked;
				if (!(src == dst) && !self->is_ancestor_of(src, dst))
				{
					world&			world = editor::get().get_app().get_world();
					entity_manager& em	  = world.get_entity_manager();
					em.remove_from_parent(src);
					em.add_child(dst, src);
					self->_tree_dirty = true;
				}
			}
			self->_is_dragging = false;
			self->_drag_source = {};
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::handled;
	}

	void editor_panel_entities::on_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button)
	{
		
	}

	bool editor_panel_entities::is_ancestor_of(world_handle ancestor, world_handle node)
	{
		if (ancestor.is_null() || node.is_null())
			return false;
		world&			w	= editor::get().get_app().get_world();
		entity_manager& em	= w.get_entity_manager();
		bool			res = false;
		em.visit_parents(node, [&res, ancestor](world_handle e) {
			if (e == ancestor)
				res = true;
		});
		return res;
	}

}
