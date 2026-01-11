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
#include "editor/editor_settings.hpp"
#include "editor/editor.hpp"

// io
#include "io/file_system.hpp"
#include "io/log.hpp"

// world
#include "world/world.hpp"
#include "world/entity_manager.hpp"

// platform
#include "platform/window.hpp"
#include "platform/process.hpp"

// resources
#include "resources/entity_template.hpp"
#include "resources/entity_template_raw.hpp"

// math
#include "math/math.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"

// misc
#include "gui/vekt.hpp"
#include "memory/bump_text_allocator.hpp"
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
		_gui_builder.callbacks.on_mouse				  = on_mouse;
		_gui_builder.callbacks.on_checkbox_changed	  = on_checkbox;
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
		_prop_vis						 = _gui_builder.add_property_row_checkbox("visible", 0).second;
		_prop_name						 = _gui_builder.add_property_row_text_field("name", "-", 256).second;
		_prop_handle					 = _gui_builder.add_property_row_label("handle:", "{-, -}", 16).second;
		const gui_builder::id_quat pos	 = _gui_builder.add_property_row_vector3("position", "0.0", 16, 3, 0.1f);
		const gui_builder::id_quat rot	 = _gui_builder.add_property_row_vector3("rotation", "0.0", 16, 3, 1.0f);
		const gui_builder::id_quat scale = _gui_builder.add_property_row_vector3("scale", "1.0", 16, 3, 0.1f);
		_selected_pos_x					 = pos.second;
		_selected_pos_y					 = pos.third;
		_selected_pos_z					 = pos.fourth;

		_selected_rot_x = rot.second;
		_selected_rot_y = rot.third;
		_selected_rot_z = rot.fourth;

		_selected_scale_x = scale.second;
		_selected_scale_y = scale.third;
		_selected_scale_z = scale.fourth;

		_gui_builder.end_area();

		_gui_builder.add_title("components");
		_components_parent = _gui_builder.begin_area(true);

		_gui_builder.add_property_row();
		_add_component	 = _gui_builder.set_fill_x(_gui_builder.add_button("add_component").first);
		_save_template	 = _gui_builder.set_fill_x(_gui_builder.add_button("save_as_template").first);
		_unlock_template = _gui_builder.set_fill_x(_gui_builder.add_button("unlock_template").first);
		_gui_builder.pop_stack();

		_gui_builder.end_area();

		_builder->widget_add_child(_builder->get_root(), _root);

		// Reserve per-entity meta storage
		_entity_meta.resize(MAX_ENTITIES);
		_node_bindings.reserve(512);
		_root_entity_widgets.reserve(512);
		_comp_remove_buttons.reserve(512);
		_add_component_buttons.reserve(512);

		_text_icon_dd_collapsed = editor::get().get_text_allocator().allocate(ICON_DD_RIGHT);
		_text_icon_dd			= editor::get().get_text_allocator().allocate(ICON_DD_DOWN);
		_text_icon_template		= editor::get().get_text_allocator().allocate(ICON_HAMMER);

		set_selected_controls();
	}

	void editor_panel_entities::uninit()
	{
		editor::get().get_text_allocator().deallocate(_text_icon_dd);
		editor::get().get_text_allocator().deallocate(_text_icon_dd_collapsed);
		editor::get().get_text_allocator().deallocate(_text_icon_template);
		_gui_builder.uninit();
	}

	void editor_panel_entities::draw(world& w, const vector2ui16& window_size)
	{
		_builder->widget_set_pos_abs(_root, vector2(0, 50));
		_builder->widget_set_size_abs(_root, vector2(window_size.x * 0.2f, window_size.y - 50));

		entity_manager& em = w.get_entity_manager();
		if (em.get_hierarchy_dirty())
		{
			rebuild_tree(w);
			em.set_hierarchy_dirty(0);
		}

		const world_handle	 selected = _selected_entity;
		bump_text_allocator& alloc	  = editor::get().get_bump_text_allocator();

		const char* name_txt = selected.is_null() ? "-" : em.get_entity_meta(selected).name;
		_gui_builder.set_text_field_text(_prop_name, name_txt, true);
		_gui_builder.set_checkbox_value(_prop_vis, selected.is_null() ? 0 : !em.get_entity_flags(selected).is_set(entity_flags::entity_flags_invisible));
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

	void editor_panel_entities::clear_component_view()
	{
		// for (vekt::id c : _component_properties)
		//	_gui_builder.deallocate(c);
		if (_components_area != NULL_WIDGET_ID)
			_gui_builder.deallocate(_components_area);
		_components_area = NULL_WIDGET_ID;

		_comp_remove_buttons.resize(0);
	}
	void editor_panel_entities::build_component_view()
	{
		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();
		if (em.get_entity_flags(_selected_entity).is_set(entity_flags::entity_flags_template))
			return;

		_gui_builder.push_stack(_components_parent);
		_components_area = _gui_builder.begin_area(false, true);

		component_manager&			cm = w.get_comp_manager();
		const entity_comp_register& cr = em.get_component_register(_selected_entity);
		for (const entity_comp& c : cr.comps)
		{
			const meta&				   m	  = reflection::get().resolve(c.comp_type);
			const meta::field_vec&	   fields = m.get_fields();
			const gui_builder::id_pair pair	  = _gui_builder.add_component_title(m.get_title().c_str());
			_comp_remove_buttons.push_back({c.comp_handle, pair.second, c.comp_type});

			for (field_base* f : fields)
			{
				_gui_builder.add_reflected_field(f, c.comp_type, cm.get_component(c.comp_type, c.comp_handle));
			}
		}

		_gui_builder.end_area();
		_gui_builder.pop_stack();
	}

	void editor_panel_entities::rebuild_tree(world& w)
	{
		const float old_scroll = _builder->widget_get_pos_props(_entity_area).scroll_offset;

		// Remove old nodes
		for (vekt::id nid : _root_entity_widgets)
		{
			_builder->deallocate(nid);
		}
		_root_entity_widgets.resize(0);
		_node_bindings.resize(0);

		_drag_source	 = {};
		_drag_src_widget = NULL_WIDGET_ID;

		entity_manager& em = w.get_entity_manager();
		// Build from roots
		for (auto it = em.get_entities()->handles_begin(); it != em.get_entities()->handles_end(); ++it)
		{
			world_handle		 h			  = *it;
			const entity_family& fam		  = em.get_entity_family(h);
			const bool			 is_transient = em.get_entity_flags(h).is_set(entity_flags::entity_flags_no_save);

			if (fam.parent.is_null() && !is_transient)
			{
				build_entity_node(w, h, 0);
			}
		}

		for (const node_binding& b : _node_bindings)
		{
			if (b.handle == _selected_entity)
			{
				_builder->set_focus(b.inner_row, false);
				break;
			}
		}

		_builder->widget_set_scroll_offset(_entity_area, old_scroll);

		if (!em.is_valid(_selected_entity))
			set_selected({});
	}

	vekt::id editor_panel_entities::build_entity_node(world& w, world_handle e, unsigned int depth)
	{
		entity_manager& em			= w.get_entity_manager();
		const uint8		is_template = em.get_entity_flags(e).is_set(entity_flags::entity_flags_template);
		const uint8		selected	= _selected_entity == e;
		if (is_template)
			_entity_meta[e.index].collapsed = true;

		const uint8 is_collapsed = _entity_meta[e.index].collapsed;

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
			hb.on_hover_move		 = on_tree_item_hover_move;
			hb.on_hover_end			 = on_tree_item_hover_end;
			hb.on_focus_gained		 = on_focus_gained;

			vekt::input_color_props& c = _builder->widget_get_input_colors(row_inner);
			c.focus_color			   = {};

			vekt::mouse_callback& mc = _builder->widget_get_mouse_callbacks(row_inner);
			mc.on_mouse				 = on_mouse;
			mc.on_drag				 = on_tree_item_drag;

			vekt::key_callback& kb = _builder->widget_get_key_callbacks(row_inner);
			kb.on_key			   = on_key;

			_builder->widget_get_user_data(row_inner).ptr = this;
		}

		const vekt::id icon_wrap = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, icon_wrap);

			vekt::pos_props& pp = _builder->widget_get_pos_props(icon_wrap);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = _builder->widget_get_size_props(icon_wrap);
			sz.flags			 = vekt::size_flags::sf_y_relative | vekt::size_flags::sf_x_copy_y;
			sz.size.y			 = 1.0f;
		}

		const vekt::id icon = _builder->allocate();
		{
			_builder->widget_add_child(icon_wrap, icon);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(icon);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = is_template ? editor_theme::get().col_accent_third : editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(icon);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_anchor_center | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.y			= 0.5f;
			pp.pos.x			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(icon);
			tp.text				 = is_template ? _text_icon_template : (is_collapsed ? _text_icon_dd_collapsed : _text_icon_dd);
			tp.font				 = editor_theme::get().font_icons;
			tp.scale			 = 0.7f;
			_builder->widget_update_text(icon);
		}

		const vekt::id txt = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, txt);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = is_template ? editor_theme::get().col_accent_third : editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.text				 = em.get_entity_meta(e).name;
			tp.font				 = editor_theme::get().font_default;
			_builder->widget_update_text(txt);
		}
		_node_bindings.push_back({row, row_inner, txt, e});
		_root_entity_widgets.push_back(row);

		const entity_family& fam = em.get_entity_family(e);
		world_handle		 ch	 = fam.first_child;

		if (!is_template)
		{
			_builder->widget_set_visible(icon, !ch.is_null());
		}

		if (!is_collapsed)
		{
			while (!ch.is_null())
			{
				build_entity_node(w, ch, depth + 1);
				ch = em.get_entity_family(ch).next_sibling;
			}
		}

		return row;
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

	void editor_panel_entities::set_collapse(world_handle h, uint8 collapsed)
	{
		_entity_meta[h.index].collapsed = collapsed;
		editor::get().get_app().get_world().get_entity_manager().set_hierarchy_dirty(1);
	}

	void editor_panel_entities::toggle_collapse(world_handle h)
	{
		uint8 collapsed = _entity_meta[h.index].collapsed;
		collapsed ^= 1;
		set_collapse(h, collapsed);
	}

	void editor_panel_entities::drop_drag(world_handle target)
	{
		if (!_is_payload_on || _drag_source.is_null())
			return;
		editor::get().get_gui_controller().disable_payload();

		world_handle src = _drag_source;
		world_handle dst = target;

		world&			world		= editor::get().get_app().get_world();
		entity_manager& em			= world.get_entity_manager();
		const bool		is_template = dst.is_null() ? false : em.get_entity_flags(dst).is_set(entity_flags::entity_flags_template);

		if (!(src == dst) && !is_ancestor_of(src, dst) && !is_template)
		{

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
		if (_selected_entity == h)
			return;

		// deselect previous
		const world_handle existing = _selected_entity;
		if (!existing.is_null())
		{
			auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [existing](const node_binding& nb) -> bool { return existing == nb.handle; });
			if (it != _node_bindings.end())
			{
				vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->inner_row);
				gfx.color			  = {0, 0, 0, 0};
			}
		}

		// select new
		_selected_entity = h;
		set_selected_controls();

		auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [h](const node_binding& nb) -> bool { return h == nb.handle; });
		if (it != _node_bindings.end())
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->inner_row);
			gfx.color			  = editor_theme::get().col_accent_second_dim;
		}

		clear_component_view();

		if (_selected_entity.is_null())
			return;

		build_component_view();
	}

	void editor_panel_entities::set_selected_controls()
	{
		bool is_template = 0;
		if (!_selected_entity.is_null())
			is_template = editor::get().get_app().get_world().get_entity_manager().get_entity_flags(_selected_entity).is_set(entity_flags::entity_flags_template);
		const bool template_disabled = _selected_entity.is_null() || is_template;

		_gui_builder.set_widget_enabled(_unlock_template, is_template, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_add_component, !template_disabled, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_save_template, !template_disabled, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_name, !template_disabled, editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_vis, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_handle, !_selected_entity.is_null(), editor_theme::get().col_text, editor_theme::get().col_button_silent);
		_builder->build_hierarchy();
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
			auto it = std::find_if(self->_node_bindings.begin(), self->_node_bindings.end(), [self](const node_binding& b) -> bool { return b.handle == self->_selected_entity; });
			if (it != self->_node_bindings.end())
			{
				b->widget_get_text(it->text).text = em.get_entity_meta(self->_selected_entity).name;
				b->widget_update_text(it->text);
			}
		}
		else if (widget == self->_prop_vis)
		{
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
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(eul.x, value, eul.z));
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

	void editor_panel_entities::on_checkbox(void* callback_ud, vekt::builder* b, vekt::id id, unsigned char value)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(callback_ud);
		if (!self->_selected_entity.is_null() && id == self->_prop_vis)
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.set_entity_visible(self->_selected_entity, value);
		}
	}

	vekt::input_event_result editor_panel_entities::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		if (ev.type == vekt::input_event_type::pressed && !self->_selected_entity.is_null() && !self->_add_component_buttons.empty())
		{
			for (const add_comp_button& b : self->_add_component_buttons)
			{
				if (b.button == widget)
				{
					world&			   w  = editor::get().get_app().get_world();
					entity_manager&	   em = w.get_entity_manager();
					component_manager& cm = w.get_comp_manager();
					cm.add_component(b.type, self->_selected_entity);
					self->clear_component_view();
					self->build_component_view();
					self->_add_component_buttons.resize(0);
					return vekt::input_event_result::handled;
				}
			}
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_save_template && !self->_selected_entity.is_null())
		{
			world&		 w	  = editor::get().get_app().get_world();
			const string file = process::save_file("save entity file", ".stkent");
			entity_template_raw::save_to_file(file.c_str(), w, {self->_selected_entity});

			string relative = editor_settings::get().get_relative(file);
			file_system::fix_path(relative);
			w.get_resource_manager().load_resources({relative});

			const resource_handle h = w.get_resource_manager().get_resource_handle_by_hash<entity_template>(TO_SID(relative));
			w.get_entity_manager().set_entity_template(self->_selected_entity, h);

			self->_entity_meta[self->_selected_entity.index].collapsed = true;
			self->set_selected_controls();
			self->clear_component_view();
			self->build_component_view();
			self->rebuild_tree(w);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_unlock_template && !self->_selected_entity.is_null())
		{
			world& w = editor::get().get_app().get_world();
			w.get_entity_manager().set_entity_template(self->_selected_entity, {});
			self->set_selected_controls();
			self->clear_component_view();
			self->build_component_view();
			self->rebuild_tree(w);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_add_component && !self->_selected_entity.is_null())
		{
			self->_add_component_buttons.resize(0);
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			const auto& metas = reflection::get().get_metas();
			for (auto& [sid, meta] : metas)
			{
				if (meta.get_tag() == "component"_hs && !meta.get_title().empty())
				{
					const vekt::id		  button = editor::get().get_gui_controller().add_context_menu_item(meta.get_title().c_str());
					vekt::mouse_callback& cb	 = b->widget_get_mouse_callbacks(button);
					cb.on_mouse					 = on_mouse;
					vekt::widget_user_data& ud	 = b->widget_get_user_data(button);
					ud.ptr						 = self;
					self->_add_component_buttons.push_back({.button = button, .type = meta.get_type_id()});
				}
			}
			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && !self->_selected_entity.is_null())
		{
			for (const comp_remove_button& b : self->_comp_remove_buttons)
			{
				if (widget == b.button)
				{
					world&			   w  = editor::get().get_app().get_world();
					entity_manager&	   em = w.get_entity_manager();
					component_manager& cm = w.get_comp_manager();
					cm.remove_component(b.comp_type, self->_selected_entity, b.handle);
					self->clear_component_view();
					self->build_component_view();

					return vekt::input_event_result::handled;
				}
			}
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_ctx_new_entity)
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");
			self->set_selected(h);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_ctx_add_child && !self->_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");

			self->_entity_meta[self->_selected_entity.index].collapsed = 0;
			em.add_child(self->_selected_entity, h);

			self->set_selected(h);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_ctx_duplicate && !self->_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.clone_entity(self->_selected_entity);
			self->set_selected(h);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed && widget == self->_ctx_delete && !self->_selected_entity.is_null())
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(self->_selected_entity);
			self->set_selected({});
			return vekt::input_event_result::handled;
		}

		if (widget == self->_entity_area)
		{
			if (ev.button == input_code::mouse_1 && ev.type == vekt::input_event_type::pressed)
			{
				editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
				self->_ctx_new_entity							   = editor::get().get_gui_controller().add_context_menu_item("new_entity");
				b->widget_get_user_data(self->_ctx_new_entity).ptr = self;

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
			if (nb.inner_row == widget)
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
			const uint8 is_template = editor::get().get_app().get_world().get_entity_manager().get_entity_flags(clicked).is_set(entity_flags::entity_flags_template);
			if (!is_template)
				self->toggle_collapse(clicked);
			self->set_selected(clicked);
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::pressed)
		{
			self->set_selected(clicked);
			self->_drag_source	   = clicked;
			self->_drag_src_widget = widget;
			self->_drag_y		   = ev.position.y;
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
			return vekt::input_event_result::handled;
		}

		if (ev.key == input_code::key_delete)
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(self->_selected_entity);
			self->set_selected({});
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	void editor_panel_entities::on_tree_item_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
	}

	void editor_panel_entities::on_tree_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		editor_panel_entities* self		   = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		b->widget_get_stroke(widget).color = {};

		if (self->_drag_src_widget == widget && !self->_is_payload_on && !self->_drag_source.is_null() && b->widget_get_hover_callbacks(self->_drag_src_widget).is_pressing)
		{
			const char* name = editor::get().get_app().get_world().get_entity_manager().get_entity_meta(self->_drag_source).name;
			editor::get().get_gui_controller().enable_payload(name);
			self->_is_payload_on = true;
		}
	}

	void editor_panel_entities::on_tree_item_hover_move(vekt::builder* b, vekt::id widget)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (self->_is_payload_on && widget != self->_drag_src_widget)
		{
			b->widget_get_stroke(widget).color = editor_theme::get().col_highlight;
		}
	}

	void editor_panel_entities::on_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		for (const node_binding& binding : self->_node_bindings)
		{
			if (binding.inner_row == widget)
			{
				self->set_selected(binding.handle);
				return;
			}
		}
	}

}
