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

#include "gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
#include "platform/window_common.hpp"
#include "platform/window.hpp"
#include "input/input_mappings.hpp"
#include "resources/entity_template.hpp"
#include "world/entity_manager.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"
namespace SFG
{
	void gameplay::on_world_begin(world& w)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		_player_entity = em.create_entity("player");
		_camera_entity = em.create_entity("player_camera");
		em.add_child(_player_entity, _camera_entity);

		_camera_comp	  = cm.add_component<comp_camera>(_camera_entity);
		comp_camera& comp = cm.get_component<comp_camera>(_camera_comp);
		comp.set_main(w);
		comp.set_values(w, 0.1, 500.0f, 90.0f);

		em.set_entity_position(_player_entity, vector3(0, 0, 15));
		em.set_entity_position(_camera_entity, vector3(0, 2, 0));
		em.set_entity_rotation(_camera_entity, quat::identity);

		_bullet_template = rm.get_resource_handle_by_hash<entity_template>(TO_SID("assets/entity_templates/bullet.stkent"));

		_direction_input	= vector3::zero;
		_mouse_delta		= vector2::zero;
		_yaw_degrees		= 0.0f;
		_pitch_degrees		= 0.0f;
		_current_move_speed = _base_move_speed;
		_is_active			= 1;

		_app.get_main_window().confine_cursor(cursor_confinement::pointer);
		_app.get_main_window().set_cursor_visible(false);
		_is_looking = true;
	}

	void gameplay::on_world_end(world& w)
	{
		entity_manager& em = w.get_entity_manager();
		if (!_player_entity.is_null())
			em.destroy_entity(_player_entity);
		_player_entity	 = {};
		_camera_entity	 = {};
		_camera_comp	 = {};
		_bullet_template = {};
		_direction_input = vector3::zero;
		_mouse_delta	 = vector2::zero;
		_is_looking		 = false;
		_is_active		 = 0;
		_app.get_main_window().confine_cursor(cursor_confinement::none);
		_app.get_main_window().set_cursor_visible(true);
	}

	void gameplay::on_world_tick(world& w, float dt)
	{
		if (_is_active == 0 || _player_entity.is_null() || _camera_entity.is_null())
			return;

		_yaw_degrees -= _mouse_delta.x * _mouse_sensitivity;
		_pitch_degrees -= _mouse_delta.y * _mouse_sensitivity;
		_pitch_degrees = math::clamp(_pitch_degrees, -89.0f, 89.0f);

		entity_manager& em		= w.get_entity_manager();
		const quat		new_rot = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		em.set_entity_rotation(_camera_entity, new_rot);

		_mouse_delta = vector2::zero;

		if (_direction_input.is_zero())
			return;

		const quat&	  rot	   = em.get_entity_rotation_abs(_camera_entity);
		const vector3 forward  = rot.get_forward();
		const vector3 right	   = rot.get_right();
		const vector3 up	   = vector3::up;
		vector3		  move_dir = (forward * _direction_input.z + right * _direction_input.x + up * _direction_input.y);

		if (move_dir.is_zero())
			return;
		move_dir.normalize();

		const vector3& pos = em.get_entity_position(_player_entity);
		auto		   p   = pos + (move_dir * (_current_move_speed * dt));
		em.set_entity_position(_player_entity, p);
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		if (_is_active == 0)
			return;

		const uint16 button = ev.button;

		switch (ev.type)
		{
		case window_event_type::focus: {
			//_direction_input	= vector3::zero;
			//_mouse_delta		= vector2::zero;
			//_is_looking			= false;
			//_current_move_speed = _base_move_speed;
			//_app.get_main_window().confine_cursor(cursor_confinement::none);
			//_app.get_main_window().set_cursor_visible(true);
			return;
		}
		case window_event_type::key: {
			if (button == input_code::key_w && ev.sub_type == window_event_sub_type::press)
				_direction_input.z += 1.0f;
			else if (button == input_code::key_w && ev.sub_type == window_event_sub_type::release && _direction_input.z > 0.1f)
				_direction_input.z -= 1.0f;
			if (button == input_code::key_s && ev.sub_type == window_event_sub_type::press)
				_direction_input.z -= 1.0f;
			else if (button == input_code::key_s && ev.sub_type == window_event_sub_type::release && _direction_input.z < -0.1f)
				_direction_input.z += 1.0f;

			if (button == input_code::key_d && ev.sub_type == window_event_sub_type::press)
				_direction_input.x += 1.0f;
			else if (button == input_code::key_d && ev.sub_type == window_event_sub_type::release && _direction_input.x > 0.1f)
				_direction_input.x -= 1.0f;
			if (button == input_code::key_a && ev.sub_type == window_event_sub_type::press)
				_direction_input.x -= 1.0f;
			else if (button == input_code::key_a && ev.sub_type == window_event_sub_type::release && _direction_input.x < -0.1f)
				_direction_input.x += 1.0f;

			if (button == input_code::key_e && ev.sub_type == window_event_sub_type::press)
				_direction_input.y += 1.0f;
			else if (button == input_code::key_e && ev.sub_type == window_event_sub_type::release && _direction_input.y > 0.1f)
				_direction_input.y -= 1.0f;
			if (button == input_code::key_q && ev.sub_type == window_event_sub_type::press)
				_direction_input.y -= 1.0f;
			else if (button == input_code::key_q && ev.sub_type == window_event_sub_type::release && _direction_input.y < -0.1f)
				_direction_input.y += 1.0f;

			if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::press)
				_current_move_speed = _base_move_speed * _boost_multiplier;
			else if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::release)
				_current_move_speed = _base_move_speed;
			break;
		}
		case window_event_type::mouse: {
			if (button == static_cast<uint16>(input_code::mouse_0) && ev.sub_type == window_event_sub_type::press)
			{
				world&			  w	 = _app.get_world();
				entity_manager&	  em = w.get_entity_manager();
				resource_manager& rm = w.get_resource_manager();
				if (rm.is_valid<entity_template>(_bullet_template))
				{
					const world_handle bullet = em.instantiate_template(_bullet_template);
					if (!bullet.is_null())
					{
						const quat&	  cam_rot	= em.get_entity_rotation_abs(_camera_entity);
						const vector3 cam_pos	= em.get_entity_position_abs(_camera_entity);
						const vector3 spawn_pos = cam_pos + (cam_rot.get_forward() * 2.0f);
						em.set_entity_position_abs(bullet, spawn_pos);
					}
				}
			}
			break;
		}

		case window_event_type::delta: {
			if (_is_looking)
			{
				_mouse_delta.x += static_cast<float>(ev.value.x);
				_mouse_delta.y += static_cast<float>(ev.value.y);
			}
			else
			{
				_mouse_delta = vector2::zero;
			}
			break;
		}
		default:
			break;
		}
	}
}
