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

#include "comp_player.hpp"
#include "world/world.hpp"
#include "platform/window.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "reflection/reflection.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/components/comp_camera.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"

#include <cmath>

namespace SFG
{
	void comp_player::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_player>::value, 0, "component");
		m.set_title("player");
		m.set_category("game");
		m.add_field<&comp_player::_movement_speed, comp_player>("movement_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_player::_rotation_speed, comp_player>("rotation_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_player::_camera_distance, comp_player>("camera_distance", reflected_field_type::rf_float, "", 0.1f, 1000.0f);
		m.add_field<&comp_player::_camera_offset, comp_player>("camera_offset", reflected_field_type::rf_vector3, "");
		m.add_field<&comp_player::_orbit_yaw_speed, comp_player>("orbit_yaw_speed", reflected_field_type::rf_float, "", 0.001f, 10.0f);
		m.add_field<&comp_player::_orbit_pitch_speed, comp_player>("orbit_pitch_speed", reflected_field_type::rf_float, "", 0.001f, 10.0f);
		m.add_field<&comp_player::_orbit_min_pitch, comp_player>("orbit_min_pitch", reflected_field_type::rf_float, "", -89.0f, 0.0f);
		m.add_field<&comp_player::_orbit_max_pitch, comp_player>("orbit_max_pitch", reflected_field_type::rf_float, "", 0.0f, 89.0f);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) { comp_player* c = static_cast<comp_player*>(params.object_ptr); });

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) { comp_player* c = static_cast<comp_player*>(obj); });
	}

	void comp_player::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
	}

	void comp_player::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
	}

	void comp_player::set_values(world& w, const color& base_color)
	{
	}

	void comp_player::begin_game(world& w, window& wnd)
	{
		_inited = false;

		entity_manager& em = w.get_entity_manager();

		_camera_entity = em.find_entity("PlayerCamera");
		if (_camera_entity.is_null())
			return;

		_camera_comp = em.get_entity_component<comp_camera>(_camera_entity);

		if (_camera_comp.is_null())
			return;

		const entity_comp_register& e = em.get_component_register(_header.entity);
		_char_controller			  = em.get_entity_component<comp_character_controller>(_header.entity);

		if (_char_controller.is_null())
			return;

		{
			const vector3 player_pos = em.get_entity_position_abs(_header.entity);
			const vector3 cam_pos	 = em.get_entity_position_abs(_camera_entity);
			const vector3 target	 = player_pos + _camera_offset;
			if (!(target - cam_pos).is_zero())
			{
				const quat	  look_rot = quat::look_at(cam_pos, target, vector3::up);
				const vector3 euler	   = quat::to_euler(look_rot);
				_yaw_degrees		   = euler.y;
				_pitch_degrees		   = euler.x;
			}
		}

		component_manager& cm = w.get_comp_manager();
		comp_camera&	   c  = cm.get_component<comp_camera>(_camera_comp);
		c.set_main(w);
		_inited = true;

		wnd.confine_cursor(cursor_confinement::window);
		wnd.set_cursor_visible(false);

		_pitch_degrees		  = -45.0f;
		_real_camera_distance = _camera_distance;
	}

	float dbg_line	= 10.0f;
	color hit_color = color::red;

	void comp_player::tick(world& w, float dt)
	{
		if (!_inited)
			return;

		entity_manager& em = w.get_entity_manager();

		component_manager&		   cm			  = w.get_comp_manager();
		comp_character_controller& comp_char_cont = cm.get_component<comp_character_controller>(_char_controller);

		_yaw_degrees -= _mouse_delta.x * _orbit_yaw_speed;
		_pitch_degrees -= _mouse_delta.y * _orbit_pitch_speed;
		_pitch_degrees = math::clamp(_pitch_degrees, _orbit_min_pitch, _orbit_max_pitch);
		_mouse_delta   = vector2::zero;

		const vector3 player_pos = em.get_entity_position_abs(_header.entity);
		const vector3 target	 = player_pos + _camera_offset;
		const quat	  orbit_rot	 = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		const vector3 forward	 = orbit_rot.get_forward();
		vector3		  forward_xz = forward;
		vector3		  right_xz	 = orbit_rot.get_right();
		forward_xz.y			 = 0.0f;
		right_xz.y				 = 0.0f;
		forward_xz.normalize();
		right_xz.normalize();
		vector3 move_dir = (forward_xz * _move_input.y) + (right_xz * _move_input.x);
		if (!move_dir.is_zero())
		{
			move_dir.normalize();
			comp_char_cont.set_target_velocity(move_dir * _movement_speed);
		}
		else
		{
			comp_char_cont.set_target_velocity(vector3::zero);
		}

		const float cam_dist		  = math::max(_camera_distance, 0.1f);
		const float cam_min_dist	  = 0.25f;
		const float cam_collision_pad = 0.2f;
		const float cam_in_speed	  = 18.0f;
		const float cam_out_speed	  = 8.0f;

		vector3 ray_dir = -forward;
		ray_dir.normalize();

		const bool res = _ray_caster.cast(w.get_physics_world(), target, ray_dir, cam_dist + cam_collision_pad);

		float desired_dist = cam_dist;
		if (res)
		{
			const ray_result& hit = _ray_caster.get_result();
			desired_dist		  = math::max(hit.hit_distance - cam_collision_pad, cam_min_dist);
			hit_color			  = color::green;
			dbg_line			  = hit.hit_distance;
		}
		else
		{
			dbg_line  = 15.0f;
			hit_color = color::red;
		}

		const float speed	  = desired_dist < _real_camera_distance ? cam_in_speed : cam_out_speed;
		const float t		  = 1.0f - std::exp(-speed * dt);
		_real_camera_distance = math::lerp(_real_camera_distance, desired_dist, t);

		vector3 desired_cam_pos = target - forward * _real_camera_distance;

		em.set_entity_position_abs(_camera_entity, desired_cam_pos);
		em.set_entity_rotation_abs(_camera_entity, orbit_rot);
	}

	void comp_player::tick_debug(world& w, float dt)
	{
		if (!_inited)
			return;

		entity_manager& em		   = w.get_entity_manager();
		const vector3	player_pos = em.get_entity_position_abs(_header.entity);
		const vector3	cam_pos	   = em.get_entity_position_abs(_camera_entity);

		w.get_debug_rendering().draw_line(cam_pos + vector3(0, -0.5, 0), player_pos, hit_color, .25f);
	}

	void comp_player::on_window_event(const window_event& ev)
	{
		if (!_inited)
			return;

		switch (ev.type)
		{
		case window_event_type::focus:
			_mouse_delta = vector2::zero;
			break;
		case window_event_type::key: {
			const uint16 button = ev.button;
			if (button == input_code::key_w && ev.sub_type == window_event_sub_type::press)
				_move_input.y += 1.0f;
			else if (button == input_code::key_w && ev.sub_type == window_event_sub_type::release && _move_input.y > 0.1f)
				_move_input.y -= 1.0f;
			if (button == input_code::key_s && ev.sub_type == window_event_sub_type::press)
				_move_input.y -= 1.0f;
			else if (button == input_code::key_s && ev.sub_type == window_event_sub_type::release && _move_input.y < -0.1f)
				_move_input.y += 1.0f;

			if (button == input_code::key_d && ev.sub_type == window_event_sub_type::press)
				_move_input.x += 1.0f;
			else if (button == input_code::key_d && ev.sub_type == window_event_sub_type::release && _move_input.x > 0.1f)
				_move_input.x -= 1.0f;
			if (button == input_code::key_a && ev.sub_type == window_event_sub_type::press)
				_move_input.x -= 1.0f;
			else if (button == input_code::key_a && ev.sub_type == window_event_sub_type::release && _move_input.x < -0.1f)
				_move_input.x += 1.0f;

			break;
		}
		case window_event_type::delta:
			_mouse_delta.x += static_cast<float>(ev.value.x);
			_mouse_delta.y += static_cast<float>(ev.value.y);
			break;
		default:
			break;
		}
	}

}
