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

#include "editor_gizmo_2d.hpp"
#include "editor/editor.hpp"
#include "editor/editor_theme.hpp"
#include "editor/gui/editor_panel_entities.hpp"
#include "world/common_entity.hpp"
#include "gui/icon_defs.hpp"
#include "gui/vekt.hpp"
#include "app/app.hpp"
#include "math/math.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"
#include "world/world.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_particle_emitter.hpp"
#include "world/components/comp_sprite.hpp"
#include "world/components/comp_audio.hpp"
#include "world/components/comp_camera.hpp"

namespace SFG
{

	namespace
	{
		bool project_point(const matrix4x4& view_proj, const vector2& panel_pos, const vector2& panel_size, const vector3& world_pos, vector2& out)
		{
			vector4 clip = view_proj * vector4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
			if (math::abs(clip.w) < MATH_EPS)
				return false;

			if (clip.w < 0.0f)
				return false;

			const float inv_w = 1.0f / clip.w;
			const float ndc_x = clip.x * inv_w;
			const float ndc_y = clip.y * inv_w;

			out.x = panel_pos.x + (ndc_x * 0.5f + 0.5f) * panel_size.x;
			out.y = panel_pos.y + (1.0f - (ndc_y * 0.5f + 0.5f)) * panel_size.y;
			return true;
		}

		void draw_icon(vekt::builder* builder, const char* icon, const vector2& screen_pos, const vector4& color)
		{
			vekt::text_props tp = {};
			tp.text				= icon;
			tp.font				= editor_theme::get().font_icons;
			tp.scale			= 2.0f;

			const vector2 text_size = vekt::builder::get_text_size(tp);
			const vector2 text_pos	= vector2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f);
			builder->add_text(tp, color, text_pos, text_size, 7, nullptr);
		}
	}

	void editor_gizmo_2d::init(vekt::builder* builder)
	{
		_builder = builder;
	}

	void editor_gizmo_2d::draw(const vector2& root_pos, const vector2& root_size, const vector2& game_render_size)
	{
		if (!_builder)
			return;

		_last_root_pos		   = root_pos;
		_last_root_size		   = root_size;
		_last_game_render_size = game_render_size;

		world&			   w  = editor::get().get_app().get_world();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		const world_handle main_cam_entity = em.get_main_camera_entity();
		const world_handle main_cam_comp   = em.get_main_camera_comp();
		if (main_cam_entity.is_null() || main_cam_comp.is_null())
			return;

		comp_camera& camera_comp = cm.get_component<comp_camera>(main_cam_comp);

		const float		aspect = _last_game_render_size.y > 0.0f ? _last_game_render_size.x / _last_game_render_size.y : 1.0f;
		const matrix4x4 view   = matrix4x4::view(em.get_entity_rotation_abs(main_cam_entity, false), em.get_entity_position_abs(main_cam_entity, false));
		const matrix4x4 proj   = matrix4x4::perspective_reverse_z(camera_comp.get_fov_degrees(), aspect, camera_comp.get_near(), camera_comp.get_far());
		_cam_view_proj		   = proj * view;

		const world_handle selected = editor::get().get_gui_controller().get_entities()->get_selected();

		cm.view<comp_point_light>([&](comp_point_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;

			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_LIGHT_BULB, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		cm.view<comp_spot_light>([&](comp_spot_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_SPOT, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		cm.view<comp_dir_light>([&](comp_dir_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_SUN, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		cm.view<comp_particle_emitter>([&](comp_particle_emitter& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_EXPLOSION, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		cm.view<comp_sprite>([&](comp_sprite& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_GUI, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		cm.view<comp_audio>([&](comp_audio& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;
			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_AUDIO, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		const world_handle editor_camera = editor::get().get_camera().get_entity();

		cm.view<comp_camera>([&](comp_camera& c) {
			const world_handle e = c.get_header().entity;

			if (e == selected || e == editor_camera)
				return comp_view_result::cont;

			const vector3 pos	 = em.get_entity_position_abs(c.get_header().entity);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_CAMERA, screen, editor_theme::get().col_text);
			return comp_view_result::cont;
		});

		auto* pool = em.get_entities();
		for (auto it = pool->handles_begin(); it != pool->handles_end(); ++it)
		{
			const world_handle handle = *it;
			if (handle == selected)
				continue;

			if (!em.get_entity_flags(handle).is_set(entity_flags_template))
				continue;

			const vector3 pos	 = em.get_entity_position_abs(handle);
			vector2		  screen = vector2::zero;
			if (project_point(_cam_view_proj, root_pos, root_size, pos, screen))
				draw_icon(_builder, ICON_HAMMER, screen, editor_theme::get().col_accent_third);
		}
	}

	bool editor_gizmo_2d::on_mouse_event(const window_event& ev)
	{
		if (ev.type != window_event_type::mouse || ev.sub_type != window_event_sub_type::press)
			return false;

		if (_last_root_size.x <= 0.0f || _last_root_size.y <= 0.0f)
			return false;

		const vector2 mp = vector2(static_cast<float>(ev.value.x), static_cast<float>(ev.value.y));
		if (mp.x < _last_root_pos.x || mp.y < _last_root_pos.y || mp.x > _last_root_pos.x + _last_root_size.x || mp.y > _last_root_pos.y + _last_root_size.y)
			return false;

		editor_panel_entities* entities = editor::get().get_gui_controller().get_entities();
		if (!entities)
			return false;

		world&			   w  = editor::get().get_app().get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();

		auto hit_test = [&](const vector3& pos, const char* icon, const world_handle& entity) {
			vector2 screen = vector2::zero;
			if (!project_point(_cam_view_proj, _last_root_pos, _last_root_size, pos, screen))
				return false;

			vekt::text_props tp = {};
			tp.text				= icon;
			tp.font				= editor_theme::get().font_icons;
			tp.scale			= 2.0f;

			const vector2 text_size = vekt::builder::get_text_size(tp);
			const vector2 text_pos	= vector2(screen.x - text_size.x * 0.5f, screen.y - text_size.y * 0.5f);
			const vector2 rect_min	= text_pos;
			const vector2 rect_max	= text_pos + text_size;
			if (mp.x >= rect_min.x && mp.x <= rect_max.x && mp.y >= rect_min.y && mp.y <= rect_max.y)
			{
				entities->set_selected(entity);
				return true;
			}
			return false;
		};

		bool handled = false;

		const world_handle selected = entities->get_selected();

		cm.view<comp_point_light>([&](comp_point_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_LIGHT_BULB, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_spot_light>([&](comp_spot_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_SPOT, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_dir_light>([&](comp_dir_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_SUN, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_particle_emitter>([&](comp_particle_emitter& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_EXPLOSION, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_sprite>([&](comp_sprite& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_GUI, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_audio>([&](comp_audio& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_AUDIO, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		cm.view<comp_camera>([&](comp_camera& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			if (handled)
				return comp_view_result::stop;
			handled = hit_test(em.get_entity_position_abs(c.get_header().entity), ICON_CAMERA, c.get_header().entity);
			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		if (handled)
			return true;

		auto* pool = em.get_entities();
		for (auto it = pool->handles_begin(); it != pool->handles_end(); ++it)
		{
			const world_handle handle = *it;
			if (handle == selected)
				continue;
			if (!em.get_entity_flags(handle).is_set(entity_flags_template))
				continue;
			if (hit_test(em.get_entity_position_abs(handle), ICON_HAMMER, handle))
				return true;
		}

		return handled;
	}
}
