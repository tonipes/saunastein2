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

#include "cutscene_ui.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/component_manager.hpp"
#include "world/components/comp_canvas.hpp"
#include "resources/font.hpp"
#include "gui/vekt.hpp"
#include "math/vector2.hpp"
#include "data/char_util.hpp"

namespace SFG
{
	void cutscene_ui::reset()
	{
		_canvas_comp = {};
		_builder = nullptr;
		_subtitle_bg = NULL_WIDGET_ID;
		_subtitle_text = NULL_WIDGET_ID;
		_subtitle_font = nullptr;
		_ready = false;
	}

	void cutscene_ui::init(world& w, world_handle camera_entity)
	{
		if (_ready)
			return;
		if (camera_entity.is_null())
			return;

		component_manager& cm = w.get_comp_manager();
		entity_manager& em = w.get_entity_manager();

		world_handle canvas_handle = em.get_entity_component<comp_canvas>(camera_entity);
		if (canvas_handle.is_null())
			canvas_handle = cm.add_component<comp_canvas>(camera_entity);

		if (canvas_handle.is_null())
			return;

		_canvas_comp = canvas_handle;

		comp_canvas& cnv = cm.get_component<comp_canvas>(_canvas_comp);
		cnv.update_counts_and_init(w, 1024, 32);

		resource_manager& rm = w.get_resource_manager();
		resource_handle font_handle = rm.get_resource_handle_by_hash_if_exists<font>("assets/fonts/roboto.stkfont"_hs);
		if (font_handle.is_null())
			return;

		font& f = rm.get_resource<font>(font_handle);
		_subtitle_font = f.get_vekt_font();
		if (_subtitle_font == nullptr)
			return;

		_builder = cnv.get_builder();
		if (_builder == nullptr)
			return;

		_subtitle_bg = _builder->allocate();
		_builder->widget_add_child(_builder->get_root(), _subtitle_bg);

		vekt::widget_gfx& bg_gfx = _builder->widget_get_gfx(_subtitle_bg);
		bg_gfx.flags = vekt::gfx_flags::gfx_is_rect;
		bg_gfx.color = VEKT_VEC4(0.0f, 0.0f, 0.0f, 0.6f);

		_builder->widget_set_pos(_subtitle_bg, VEKT_VEC2(0.5f, 0.92f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::center, vekt::helper_anchor_type::center);
		_builder->widget_set_size_abs(_subtitle_bg, VEKT_VEC2(400.0f, 60.0f));

		_subtitle_text = _builder->allocate();
		_builder->widget_add_child(_subtitle_bg, _subtitle_text);

		vekt::widget_gfx& txt_gfx = _builder->widget_get_gfx(_subtitle_text);
		txt_gfx.flags = vekt::gfx_flags::gfx_is_text;
		txt_gfx.color = VEKT_VEC4(1.0f, 1.0f, 1.0f, 1.0f);

		vekt::text_props& tp = _builder->widget_get_text(_subtitle_text);
		tp.font = _subtitle_font;
		tp.scale = 1.0f;

		_builder->widget_set_text(_subtitle_text, "", 512);
		_builder->widget_set_pos(_subtitle_text, VEKT_VEC2(0.5f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::center, vekt::helper_anchor_type::center);

		_builder->build_hierarchy();
		_ready = true;
	}

	void cutscene_ui::set_subtitle_text(const char* text)
	{
		if (!_ready || _builder == nullptr || _subtitle_text == NULL_WIDGET_ID || _subtitle_bg == NULL_WIDGET_ID)
			return;
		if (_subtitle_font == nullptr)
			return;

		const char* subtitle_text = text == nullptr ? "" : text;

		vekt::text_props& tp = _builder->widget_get_text(_subtitle_text);
		if (tp.text == nullptr)
		{
			_builder->widget_set_text(_subtitle_text, subtitle_text, 512);
		}
		else
		{
			char* start = const_cast<char*>(tp.text);
			char* end = start + tp.text_capacity;
			start[0] = '\0';
			char* cur = start;
			SFG::char_util::append(cur, end, subtitle_text);
			if (cur == end)
				end[-1] = '\0';
			_builder->widget_update_text(_subtitle_text);
		}

		const vector2 text_size = _builder->get_text_size(tp);
		_builder->widget_set_size_abs(_subtitle_bg, VEKT_VEC2(text_size.x + 40.0f, text_size.y + 20.0f));
	}
}
