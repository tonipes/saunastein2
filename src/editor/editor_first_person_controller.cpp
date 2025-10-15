// Copyright (c) 2025 Inan Evin

#include "editor_first_person_controller.hpp"

#include <algorithm>

#include "platform/window_common.hpp"
#include "platform/window.hpp"
#include "input/input_mappings.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"

namespace SFG
{
	void editor_first_person_controller::init(world& world, world_handle entity, window* wnd)
	{
		_window		 = wnd;
		_world		 = &world;
		_entity		 = entity;
		_mouse_delta = vector2::zero;

		if (_entity.is_null())
			return;

		entity_manager& manager = world.get_entity_manager();
		const quat&		rot		= manager.get_entity_rotation(entity);
		const vector3	euler	= quat::to_euler(rot);
		_yaw_degrees			= euler.x;
		_pitch_degrees			= euler.y;
	}

	void editor_first_person_controller::uninit()
	{
		_entity = {};
		reset_runtime();
	}

	void editor_first_person_controller::reset_runtime()
	{
		_direction_input = vector3::zero;
		_mouse_delta	 = vector2::zero;
		_is_looking		 = false;
		_window->set_cursor_visible(true);
	}

	void editor_first_person_controller::on_window_event(const window_event& ev)
	{
		if (!_world || _entity.is_null())
			return;

		const uint16 button = ev.button;

		switch (ev.type)
		{
		case window_event_type::focus: {
			reset_runtime();
			break;
		}
		case window_event_type::key: {

			if (button == input_code::key_w && ev.sub_type == window_event_sub_type::press)
				_direction_input.z += 1.0f;
			else if (button == input_code::key_w && ev.sub_type == window_event_sub_type::release)
				_direction_input.z -= 1.0f;
			if (button == input_code::key_s && ev.sub_type == window_event_sub_type::press)
				_direction_input.z -= 1.0f;
			else if (button == input_code::key_s && ev.sub_type == window_event_sub_type::release)
				_direction_input.z += 1.0f;

			if (button == input_code::key_d && ev.sub_type == window_event_sub_type::press)
				_direction_input.x += 1.0f;
			else if (button == input_code::key_d && ev.sub_type == window_event_sub_type::release)
				_direction_input.x -= 1.0f;
			if (button == input_code::key_a && ev.sub_type == window_event_sub_type::press)
				_direction_input.x -= 1.0f;
			else if (button == input_code::key_a && ev.sub_type == window_event_sub_type::release)
				_direction_input.x += 1.0f;

			if (button == input_code::key_e && ev.sub_type == window_event_sub_type::press)
				_direction_input.y += 1.0f;
			else if (button == input_code::key_e && ev.sub_type == window_event_sub_type::release)
				_direction_input.y -= 1.0f;
			if (button == input_code::key_q && ev.sub_type == window_event_sub_type::press)
				_direction_input.y -= 1.0f;
			else if (button == input_code::key_q && ev.sub_type == window_event_sub_type::release)
				_direction_input.y += 1.0f;

			if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::press)
				_current_move_speed = _base_move_speed * _boost_multiplier;
			else if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::release)
				_current_move_speed = _base_move_speed;

			break;
		}
		case window_event_type::mouse: {
			if (button == static_cast<uint16>(input_code::mouse_1))
			{
				const bool was_looking = _is_looking;

				if (ev.sub_type == window_event_sub_type::press || ev.sub_type == window_event_sub_type::repeat)
				{
					_window->confine_cursor(cursor_confinement::pointer);
					_window->set_cursor_visible(false);
					_is_looking = true;
				}
				else if (ev.sub_type == window_event_sub_type::release)
				{
					_window->confine_cursor(cursor_confinement::none);
					_window->set_cursor_visible(true);
					_is_looking	 = false;
					_mouse_delta = vector2::zero;
				}
			}
			break;
		}
		case window_event_type::wheel:
			break;
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

	void editor_first_person_controller::tick(float dt_seconds)
	{
		if (!_world || _entity.is_null())
			return;

		update_rotation();
		apply_movement(dt_seconds);
		_world->get_entity_manager().teleport_entity(_entity);
	}

	void editor_first_person_controller::update_rotation()
	{
		if (!_world || _entity.is_null())
			return;

		if (_mouse_delta.is_zero())
			return;

		_yaw_degrees -= _mouse_delta.x * _mouse_sensitivity;
		_pitch_degrees -= _mouse_delta.y * _mouse_sensitivity;
		_pitch_degrees = math::clamp(_pitch_degrees, -89.0f, 89.0f);

		entity_manager& manager = _world->get_entity_manager();
		const quat		new_rot = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		manager.set_entity_rotation(_entity, new_rot);

		_mouse_delta = vector2::zero;
	}

	void editor_first_person_controller::apply_movement(float dt_seconds)
	{
		if (!_world || _entity.is_null())
			return;

		auto is_down = [this](uint16 code) -> bool { return false; };

		entity_manager& manager = _world->get_entity_manager();
		const quat&		rot		= manager.get_entity_rotation(_entity);

		const vector3 forward  = rot.get_forward();
		const vector3 right	   = rot.get_right();
		const vector3 up	   = vector3::up;
		vector3		  move_dir = (forward * _direction_input.z + right * _direction_input.x + up * _direction_input.y);

		if (move_dir.is_zero())
			return;
		move_dir.normalize();

		const vector3 position = manager.get_entity_position(_entity) + (move_dir * (_current_move_speed * dt_seconds));
		manager.set_entity_position(_entity, position);
	}
}
