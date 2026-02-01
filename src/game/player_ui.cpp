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

#include "player_ui.hpp"
#include "math/math.hpp"
#include "math/vector2.hpp"
#include "data/char_util.hpp"
#include "game/gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "resources/font.hpp"

namespace SFG
{
#define OFFSET 20

	void player_ui::init(vekt::builder* builder)
	{
		if (builder == nullptr)
			return;

		_builder = builder;

		world& w  = gameplay::get().get_app().get_world();
		auto&  rm = w.get_resource_manager();

		vekt::font* text_font	  = nullptr;
		vekt::font* subtitle_font = nullptr;

		resource_handle text_font_handle = rm.get_resource_handle_by_hash_if_exists<font>("assets/fonts/quantico.stkfont"_hs);
		if (!text_font_handle.is_null())
		{
			font& f	  = rm.get_resource<font>(text_font_handle);
			text_font = f.get_vekt_font();
		}

		resource_handle st_font_handle = rm.get_resource_handle_by_hash_if_exists<font>("assets/fonts/quantico_big.stkfont"_hs);
		if (!st_font_handle.is_null())
		{
			font& f		  = rm.get_resource<font>(st_font_handle);
			subtitle_font = f.get_vekt_font();
		}

		if (_crosshair == NULL_WIDGET_ID)
		{
			_crosshair = _builder->allocate();
			_builder->widget_add_child(_builder->get_root(), _crosshair);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_crosshair);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = VEKT_VEC4(1.0f, 1.0f, 1.0f, 1.0f);

			vekt::size_props& sz = _builder->widget_get_size_props(_crosshair);
			sz.size.y			 = 0.0025f;
			sz.flags			 = vekt::size_flags::sf_y_relative | vekt::size_flags::sf_x_copy_y;

			_builder->widget_set_pos(_crosshair, VEKT_VEC2(0.5f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::center, vekt::helper_anchor_type::center);
		}

		if (_health_bg == NULL_WIDGET_ID)
		{
			_health_bg = _builder->allocate();
			_builder->widget_add_child(_builder->get_root(), _health_bg);

			vekt::widget_gfx& bg_gfx = _builder->widget_get_gfx(_health_bg);
			bg_gfx.flags			 = vekt::gfx_flags::gfx_is_rect;
			bg_gfx.color			 = VEKT_VEC4(0.0f, 0.0f, 0.0f, 0.65f);

			vekt::pos_props& pp = _builder->widget_get_pos_props(_health_bg);
			pp.flags			= vekt::pos_flags::pf_x_abs | vekt::pos_flags::pf_y_abs;

			vekt::size_props& bg_sz = _builder->widget_get_size_props(_health_bg);
			bg_sz.size.x			= 0.2f;
			bg_sz.size.y			= OFFSET;
			bg_sz.flags				= vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_abs;
			bg_sz.child_margins		= {2.0f, 2.0f, 2.0f, 2.0f};

			_health_bar = _builder->allocate();
			_builder->widget_add_child(_health_bg, _health_bar);

			vekt::widget_gfx& bar_gfx = _builder->widget_get_gfx(_health_bar);
			bar_gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			bar_gfx.color			  = VEKT_VEC4(0.85f, 0.1f, 0.1f, 0.95f);

			_builder->widget_set_size(_health_bar, VEKT_VEC2(1.0f, 1.0f), vekt::helper_size_type::relative, vekt::helper_size_type::relative);
			_builder->widget_set_pos(_health_bar, VEKT_VEC2(0.0f, 0.0f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::start);

			{
				vekt::id title = _builder->allocate();
				_builder->widget_add_child(_health_bg, title);

				vekt::pos_props& p = _builder->widget_get_pos_props(title);
				p.flags			   = vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
				p.pos.x			   = 0.05f;
				p.pos.y			   = 0.5f;

				auto& g		 = _builder->widget_get_gfx(title);
				g.flags		 = vekt::gfx_flags::gfx_is_text;
				g.draw_order = 2;

				auto& t = _builder->widget_get_text(title);
				t.font	= text_font;

				_builder->widget_set_text(title, "Health");
			}
		}

		if (_hydration_bg == NULL_WIDGET_ID)
		{
			_hydration_bg = _builder->allocate();
			_builder->widget_add_child(_builder->get_root(), _hydration_bg);

			vekt::widget_gfx& bg_gfx = _builder->widget_get_gfx(_hydration_bg);
			bg_gfx.flags			 = vekt::gfx_flags::gfx_is_rect;
			bg_gfx.color			 = VEKT_VEC4(0.0f, 0.0f, 0.0f, 0.65f);

			vekt::pos_props& pp = _builder->widget_get_pos_props(_hydration_bg);
			pp.flags			= vekt::pos_flags::pf_x_abs | vekt::pos_flags::pf_y_abs;

			vekt::size_props& bg_sz = _builder->widget_get_size_props(_hydration_bg);
			bg_sz.size.x			= 0.2f;
			bg_sz.size.y			= OFFSET;
			bg_sz.flags				= vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_abs;
			bg_sz.child_margins		= {2.0f, 2.0f, 2.0f, 2.0f};

			_hydration_bar = _builder->allocate();
			_builder->widget_add_child(_hydration_bg, _hydration_bar);

			vekt::widget_gfx& bar_gfx = _builder->widget_get_gfx(_hydration_bar);
			bar_gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			bar_gfx.color			  = VEKT_VEC4(0.1f, 0.45f, 0.9f, 0.95f);

			_builder->widget_set_size(_hydration_bar, VEKT_VEC2(1.0f, 1.0f), vekt::helper_size_type::relative, vekt::helper_size_type::relative);
			_builder->widget_set_pos(_hydration_bar, VEKT_VEC2(0.0f, 0.0f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::start);

			{
				vekt::id title = _builder->allocate();
				_builder->widget_add_child(_hydration_bg, title);

				vekt::pos_props& p = _builder->widget_get_pos_props(title);
				p.flags			   = vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
				p.pos.x			   = 0.05f;
				p.pos.y			   = 0.5f;

				auto& g		 = _builder->widget_get_gfx(title);
				g.flags		 = vekt::gfx_flags::gfx_is_text;
				g.draw_order = 2;

				auto& t = _builder->widget_get_text(title);
				t.font	= text_font;

				_builder->widget_set_text(title, "Hydration");
			}
		}

		if (_subtitle_bg == NULL_WIDGET_ID)
		{
			_subtitle_bg = _builder->allocate();
			_builder->widget_add_child(_builder->get_root(), _subtitle_bg);

			vekt::widget_gfx& bg_gfx = _builder->widget_get_gfx(_subtitle_bg);
			bg_gfx.flags			 = vekt::gfx_flags::gfx_is_rect;
			bg_gfx.color			 = VEKT_VEC4(0.0f, 0.0f, 0.0f, 0.6f);

			_builder->widget_set_pos(_subtitle_bg, VEKT_VEC2(0.5f, 0.92f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::center, vekt::helper_anchor_type::center);
			_builder->widget_set_size_abs(_subtitle_bg, VEKT_VEC2(400.0f, OFFSET));

			_subtitle_text = _builder->allocate();
			_builder->widget_add_child(_subtitle_bg, _subtitle_text);

			vekt::widget_gfx& txt_gfx = _builder->widget_get_gfx(_subtitle_text);
			txt_gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			txt_gfx.color			  = VEKT_VEC4(1.0f, 1.0f, 1.0f, 1.0f);

			vekt::text_props& tp = _builder->widget_get_text(_subtitle_text);
			tp.font				 = subtitle_font;
			_builder->widget_set_text(_subtitle_text, "", 512);
			_builder->widget_set_pos(_subtitle_text, VEKT_VEC2(0.5f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::center, vekt::helper_anchor_type::center);

			_builder->widget_set_visible(_subtitle_bg, false);
			_builder->widget_set_visible(_subtitle_text, false);
		}

		_builder->build_hierarchy();
	}

	void player_ui::uninit()
	{
		return;

		if (_builder == nullptr)
			return;

		if (_health_bar != NULL_WIDGET_ID)
			_builder->deallocate(_health_bar);
		if (_health_bg != NULL_WIDGET_ID)
			_builder->deallocate(_health_bg);
		if (_crosshair != NULL_WIDGET_ID)
			_builder->deallocate(_crosshair);
		if (_hydration_bar != NULL_WIDGET_ID)
			_builder->deallocate(_hydration_bar);
		if (_hydration_bg != NULL_WIDGET_ID)
			_builder->deallocate(_hydration_bg);

		_health_bar	   = NULL_WIDGET_ID;
		_health_bg	   = NULL_WIDGET_ID;
		_crosshair	   = NULL_WIDGET_ID;
		_hydration_bar = NULL_WIDGET_ID;
		_hydration_bg  = NULL_WIDGET_ID;
		_builder	   = nullptr;
	}

	void player_ui::set_health_fraction(float health_fraction)
	{
		if (_builder == nullptr || _health_bar == NULL_WIDGET_ID)
			return;

		const float clamped = math::clamp(health_fraction, 0.0f, 1.0f);
		_builder->widget_set_size(_health_bar, VEKT_VEC2(clamped, 1.0f), vekt::helper_size_type::relative, vekt::helper_size_type::relative);
	}

	void player_ui::set_hydration_fraction(float hydration_fraction)
	{
		if (_builder == nullptr || _hydration_bar == NULL_WIDGET_ID)
			return;

		const float clamped = math::clamp(hydration_fraction, 0.0f, 1.0f);
		_builder->widget_set_size(_hydration_bar, VEKT_VEC2(clamped, 1.0f), vekt::helper_size_type::relative, vekt::helper_size_type::relative);
	}

	void player_ui::set_subtitle_text(const char* text)
	{
		if (_builder == nullptr || _subtitle_text == NULL_WIDGET_ID || _subtitle_bg == NULL_WIDGET_ID)
			return;

		const char* subtitle_text = text;
		if (subtitle_text == nullptr || subtitle_text[0] == '\0')
			subtitle_text = "TEST SUBTITLE";

		vekt::text_props& tp = _builder->widget_get_text(_subtitle_text);

		if (tp.text == nullptr)
		{
			_builder->widget_set_text(_subtitle_text, subtitle_text, 512);
		}
		else
		{
			char* start = const_cast<char*>(tp.text);
			char* end	= start + tp.text_capacity;
			start[0]	= '\0';
			char* cur	= start;
			SFG::char_util::append(cur, end, subtitle_text);
			if (cur == end)
				end[-1] = '\0';
			_builder->widget_update_text(_subtitle_text);
		}

		const vector2 text_size = _builder->get_text_size(tp);
		_builder->widget_set_size_abs(_subtitle_bg, VEKT_VEC2(text_size.x + 40.0f, text_size.y + 20.0f));

		_builder->widget_set_visible(_subtitle_bg, true);
		_builder->widget_set_visible(_subtitle_text, true);
	}
	void player_ui::tick()
	{
		auto res = gameplay::get().get_app().get_world().get_screen().get_world_resolution();

		{
			auto& pp = _builder->widget_get_pos_props(_health_bg);
			pp.pos.x = OFFSET;
			pp.pos.y = res.y - OFFSET - _builder->widget_get_size(_health_bg).y;
		}

		{
			auto& pp = _builder->widget_get_pos_props(_hydration_bg);
			pp.pos.x = OFFSET;
			pp.pos.y = res.y - OFFSET - _builder->widget_get_size(_health_bg).y - OFFSET - _builder->widget_get_size(_hydration_bg).y;
		}
	}
}
