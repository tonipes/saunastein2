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

#include "platform/window.hpp"

#include "math/math.hpp"
#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "gui/vekt.hpp"
#include "memory/bump_text_allocator.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "gui/icon_defs.hpp"
#include "input/input_mappings.hpp"
#include "app/app.hpp"

#include "reflection/reflection.hpp"
namespace SFG
{

	void editor_panel_entities::init(vekt::builder* b)
	{
		_builder = b;

		// gui_builder now uses editor_theme directly for styles and fonts.
		_gui_builder.init(b);

		_gui_builder.callbacks.user_data			  = this;
		_gui_builder.callbacks.callback_ud			  = this;
		_gui_builder.callbacks.on_input_field_changed = on_input_field_changed;
		_root										  = _gui_builder.get_root();

		_gui_builder.add_title("entities");
		_entity_area													 = _gui_builder.begin_area(true);
		_builder->widget_get_size_props(_entity_area).child_margins.left = 0.0f;
		_builder->widget_get_size_props(_entity_area).spacing			 = 0.0f;
		_builder->widget_get_hover_callbacks(_entity_area).receive_mouse = 1;
		_builder->widget_get_mouse_callbacks(_entity_area).on_mouse		 = on_mouse;
		_gui_builder.end_area();

		// Properties section
		_gui_builder.add_title("properties");
		_gui_builder.begin_area(false);
		_prop_name						 = _gui_builder.add_property_row_text_field("name", "-", 256).first;
		_prop_handle					 = _gui_builder.add_property_row_label("handle:", "{-, -}", 16).second;
		const gui_builder::id_trip pos	 = _gui_builder.add_property_row_vector3("position", "0.0", 16, 3, 0.1f);
		const gui_builder::id_trip rot	 = _gui_builder.add_property_row_vector3("rotation", "0.0", 16, 3, 0.1f);
		const gui_builder::id_trip scale = _gui_builder.add_property_row_vector3("scale", "1.0", 16, 3, 0.1f);
		_selected_pos_x					 = pos.first;
		_selected_pos_y					 = pos.second;
		_selected_pos_z					 = pos.third;

		_selected_rot_x = rot.first;
		_selected_rot_y = rot.second;
		_selected_rot_z = rot.third;

		_selected_scale_x = scale.first;
		_selected_scale_y = scale.second;
		_selected_scale_z = scale.third;

		_gui_builder.end_area();

		_gui_builder.add_title("components");
		_components_area = _gui_builder.begin_area(true);
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

		const world_handle	 selected = _selected_entity;
		bump_text_allocator& alloc	  = editor::get().get_bump_text_allocator();
		entity_manager&		 em		  = w.get_entity_manager();

		const char* name_txt = selected.is_null() ? "-" : em.get_entity_meta(selected).name;
		_gui_builder.set_text_field_text(_prop_name, name_txt, true);

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

		// transform
		{
			if (selected.is_null())
			{
				_gui_builder.set_text_field_value(_selected_pos_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_pos_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_pos_z, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_z, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_z, 0.0f, true);
			}
			else
			{
				entity_manager& em	  = w.get_entity_manager();
				const vector3&	pos	  = em.get_entity_position(selected);
				const quat&		rot	  = em.get_entity_rotation(selected);
				const vector3&	scale = em.get_entity_scale(selected);
				const vector3&	eul	  = quat::to_euler(rot);
				_gui_builder.set_text_field_value(_selected_pos_x, pos.x, true);
				_gui_builder.set_text_field_value(_selected_pos_y, pos.y, true);
				_gui_builder.set_text_field_value(_selected_pos_z, pos.z, true);
				_gui_builder.set_text_field_value(_selected_rot_x, eul.x, true);
				_gui_builder.set_text_field_value(_selected_rot_y, eul.y, true);
				_gui_builder.set_text_field_value(_selected_rot_z, eul.z, true);
				_gui_builder.set_text_field_value(_selected_scale_x, scale.x, true);
				_gui_builder.set_text_field_value(_selected_scale_y, scale.y, true);
				_gui_builder.set_text_field_value(_selected_scale_z, scale.z, true);
			}
		}
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

		for (const node_binding& b : _node_bindings)
		{
			if (b.handle == _selected_entity)
			{
				_builder->set_focus(b.widget, false);
				break;
			}
		}
	}

	void editor_panel_entities::build_entity_node(world& w, world_handle e, unsigned int depth)
	{
		entity_manager& em			 = w.get_entity_manager();
		const bool		is_collapsed = _entity_meta[e.index].collapsed == 1;
		const bool		selected	 = _selected_entity == e;

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
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_rounding | vekt::gfx_flags::gfx_has_stroke | vekt::gfx_flags::gfx_focusable;
			gfx.color			  = selected ? editor_theme::get().col_accent_second_dim : (vector4());

			vekt::stroke_props& sp = _builder->widget_get_stroke(row_inner);
			sp.thickness		   = editor_theme::get().frame_thickness;
			sp.color			   = vector4(0.0f, 0.0f, 0.0f, 0.0f);

			vekt::rounding_props& rp = _builder->widget_get_rounding(row_inner);
			rp.rounding				 = editor_theme::get().frame_rounding;
			rp.segments				 = 12;

			vekt::hover_callback& hb = _builder->widget_get_hover_callbacks(row_inner);
			hb.receive_mouse		 = 1;
			hb.on_hover_begin		 = on_tree_item_hover_begin;
			hb.on_hover_end			 = on_tree_item_hover_end;
			hb.on_focus_gained		 = on_focus_gained;

			vekt::mouse_callback& mc = _builder->widget_get_mouse_callbacks(row_inner);
			mc.on_mouse				 = on_mouse;
			mc.on_drag				 = on_drag;

			vekt::key_callback& kb = _builder->widget_get_key_callbacks(row_inner);
			kb.on_key			   = on_key;

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

	void editor_panel_entities::drop_drag(world_handle target)
	{
		if (!_is_payload_on || _drag_source.is_null())
			return;
		editor::get().get_gui_controller().disable_payload();

		world_handle src = _drag_source;
		world_handle dst = target;

		if (!(src == dst) && !is_ancestor_of(src, dst))
		{
			world&			world = editor::get().get_app().get_world();
			entity_manager& em	  = world.get_entity_manager();

			const vector3 pos	= em.get_entity_position_abs(src);
			const quat	  rot	= em.get_entity_rotation_abs(src);
			const vector3 scale = em.get_entity_scale_abs(src);

			if (!em.get_entity_family(src).parent.is_null())
				em.remove_from_parent(src);

			if (!dst.is_null())
				em.add_child(dst, src);

			em.set_entity_position_abs(src, pos);
			em.set_entity_rotation_abs(src, rot);
			em.set_entity_scale_abs(src, scale);
			set_tree_dirty();
		}
		_is_payload_on = false;
		_drag_source   = {};
	}

	void editor_panel_entities::kill_drag()
	{
		_is_payload_on	 = false;
		_drag_src_widget = {};
	}

	void editor_panel_entities::set_selected(world_handle h)
	{
		// deselect previous
		const world_handle existing = _selected_entity;
		if (!existing.is_null())
		{
			auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [existing](const node_binding& nb) -> bool { return existing == nb.handle; });
			if (it != _node_bindings.end())
			{
				vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->widget);
				gfx.color			  = {0, 0, 0, 0};
			}
		}

		// select new
		_selected_entity = h;
		auto it			 = std::find_if(_node_bindings.begin(), _node_bindings.end(), [h](const node_binding& nb) -> bool { return h == nb.handle; });
		if (it != _node_bindings.end())
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->widget);
			gfx.color			  = editor_theme::get().col_accent_second_dim;
		}

		if (_selected_entity.is_null())
			return;

		world&						w  = editor::get().get_app().get_world();
		entity_manager&				em = w.get_entity_manager();
		component_manager&			cm = w.get_comp_manager();
		const entity_comp_register& cr = em.get_component_register(h);
		for (const entity_comp& c : cr.comps)
		{
			const meta&			   m	  = reflection::get().resolve(c.comp_type);
			const meta::field_vec& fields = m.get_fields();
			for (const field_base* f : fields)
			{
				if (f->_type == reflected_field_type::rf_float_clamped)
				{
					_gui_builder.push_stack(_components_area);
					_gui_builder.add_property_row_slider(f->_title.c_str(), 0, f->_min, f->_max, 0.0f);
					_gui_builder.pop_stack();
				}
			}
		}
	}

	void editor_panel_entities::on_input_field_changed(void* callback_ud, vekt::builder* b, vekt::id widget, const char* txt, float value)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(callback_ud);

		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();

		if (!em.is_valid(self->_selected_entity))
			return;

		const vector3& pos	 = em.get_entity_position(self->_selected_entity);
		const quat&	   rot	 = em.get_entity_rotation(self->_selected_entity);
		const vector3& scale = em.get_entity_scale(self->_selected_entity);
		const vector3& eul	 = quat::to_euler(rot);

		if (widget == self->_prop_name)
		{
			em.set_entity_name(self->_selected_entity, txt);
			self->set_tree_dirty();
		}
		else if (widget == self->_selected_pos_x)
		{
			em.set_entity_position(self->_selected_entity, vector3(value, pos.y, pos.z));
		}
		else if (widget == self->_selected_pos_y)
		{
			em.set_entity_position(self->_selected_entity, vector3(pos.x, value, pos.z));
		}
		else if (widget == self->_selected_pos_z)
		{
			em.set_entity_position(self->_selected_entity, vector3(pos.x, pos.y, value));
		}
		else if (widget == self->_selected_rot_x)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(value, eul.y, eul.z));
		}
		else if (widget == self->_selected_rot_y)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(eul.x, eul.y, eul.z));
		}
		else if (widget == self->_selected_rot_z)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(eul.x, eul.y, value));
		}
		else if (widget == self->_selected_scale_x)
		{
			em.set_entity_scale(self->_selected_entity, vector3(value, scale.y, scale.z));
		}
		else if (widget == self->_selected_scale_y)
		{
			em.set_entity_scale(self->_selected_entity, vector3(scale.x, value, scale.z));
		}
		else if (widget == self->_selected_scale_z)
		{
			em.set_entity_scale(self->_selected_entity, vector3(scale.x, scale.y, value));
		}
	}

	vekt::input_event_result editor_panel_entities::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		SFG_TRACE("widget {0} {1}", widget, (uint32)ev.type);

		if (widget == self->_ctx_new_entity)
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");
			self->set_selected(h);
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_add_child && !self->_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");

			self->_entity_meta[self->_selected_entity.index].collapsed = 0;
			self->set_tree_dirty();
			em.add_child(self->_selected_entity, h);

			self->set_selected(h);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_duplicate && !self->_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.clone_entity(self->_selected_entity);
			self->set_tree_dirty();
			self->set_selected(h);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_delete && !self->_selected_entity.is_null())
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(self->_selected_entity);
			self->set_selected({});
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_entity_area)
		{
			if (ev.button == input_code::mouse_1 && ev.type == vekt::input_event_type::pressed)
			{
				editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
				self->_ctx_new_entity							   = editor::get().get_gui_controller().add_context_menu_item("new_entity");
				b->widget_get_user_data(self->_ctx_new_entity).ptr = self;

				SFG_TRACE("new e {0}", self->_ctx_new_entity);
				b->widget_get_mouse_callbacks(self->_ctx_new_entity).on_mouse = on_mouse;

				editor::get().get_gui_controller().end_context_menu();
				return vekt::input_event_result::handled;
			}

			if (ev.button == input_code::mouse_0 && ev.type == vekt::input_event_type::pressed)
			{
				self->set_selected({});
				return vekt::input_event_result::handled;
			}

			if (ev.button == input_code::mouse_0 && ev.type == vekt::input_event_type::released)
			{
				if (self->_is_payload_on)
					self->drop_drag({});
				return vekt::input_event_result::handled;
			}

			return vekt::input_event_result::not_handled;
		}

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
			self->_ctx_add_child = editor::get().get_gui_controller().add_context_menu_item("add_child");
			self->_ctx_duplicate = editor::get().get_gui_controller().add_context_menu_item("duplicate");
			self->_ctx_delete	 = editor::get().get_gui_controller().add_context_menu_item("delete");

			b->widget_get_mouse_callbacks(self->_ctx_add_child).on_mouse = on_mouse;
			b->widget_get_mouse_callbacks(self->_ctx_duplicate).on_mouse = on_mouse;
			b->widget_get_mouse_callbacks(self->_ctx_delete).on_mouse	 = on_mouse;
			b->widget_get_user_data(self->_ctx_add_child).ptr			 = self;
			b->widget_get_user_data(self->_ctx_duplicate).ptr			 = self;
			b->widget_get_user_data(self->_ctx_delete).ptr				 = self;

			editor::get().get_gui_controller().end_context_menu();

			self->set_selected(clicked);

			return vekt::input_event_result::handled;
		}

		if (ev.button != input_code::mouse_0)
			return vekt::input_event_result::not_handled;

		if (ev.type == vekt::input_event_type::repeated)
		{
			self->_entity_meta[clicked.index].collapsed ^= 1;
			self->set_selected(clicked);
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed)
		{
			self->set_selected(clicked);
			self->_drag_source = clicked;
			self->_drag_y	   = ev.position.y;
			return vekt::input_event_result::handled;
		}

		if (self->_is_payload_on && ev.type == vekt::input_event_type::released)
		{
			if (self->_is_payload_on)
				self->drop_drag(clicked);
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::handled;
	}

	vekt::input_event_result editor_panel_entities::on_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (self->_selected_entity.is_null())
			return vekt::input_event_result::not_handled;

		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		if (ev.key == input_code::key_d && window::is_key_down(input_code::key_lctrl))
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.clone_entity(self->_selected_entity);
			self->set_selected(h);
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		if (ev.key == input_code::key_delete)
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(self->_selected_entity);
			self->set_selected({});
			self->set_tree_dirty();
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	void editor_panel_entities::on_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (math::abs(mp_y - self->_drag_y) && !self->_drag_source.is_null())
		{
			const char* name = editor::get().get_app().get_world().get_entity_manager().get_entity_meta(self->_drag_source).name;
			editor::get().get_gui_controller().enable_payload(name);
			self->_is_payload_on = true;
		}
	}

	void editor_panel_entities::on_tree_item_hover_begin(vekt::builder* b, vekt::id widget)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (self->_is_payload_on && widget != self->_drag_src_widget)
			b->widget_get_stroke(widget).color = editor_theme::get().col_highlight;
	}

	void editor_panel_entities::on_tree_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_stroke(widget).color = {};
	}

	void editor_panel_entities::on_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		for (const node_binding& binding : self->_node_bindings)
		{
			if (binding.widget == widget)
			{
				self->set_selected(binding.handle);
				return;
			}
		}
	}

}
