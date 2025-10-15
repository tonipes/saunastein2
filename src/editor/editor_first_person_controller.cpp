// Copyright (c) 2025 Inan Evin

#include "editor_first_person_controller.hpp"

#include <algorithm>

#include "platform/window.hpp"
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"

namespace SFG
{
	void editor_first_person_controller::init(world& world, world_handle entity, window* win)
	{
		_world	= &world;
		_entity = entity;
		_window = win;
		std::fill(_key_states.begin(), _key_states.end(), false);
		_mouse_delta = vector2::zero;

		if (_entity.is_null())
			return;

		entity_manager& manager = world.get_entity_manager();
		const quat&		rot		= manager.get_entity_rotation_abs(entity);
		const vector3	euler	= quat::to_euler(rot);
		_yaw_degrees			= euler.x;
		_pitch_degrees			= euler.y;

		std::fill(_key_states.begin(), _key_states.end(), false);
	}

	void editor_first_person_controller::uninit()
	{
		_entity		 = {};
		_mouse_delta = vector2::zero;
		_is_looking	 = false;
		_window		 = nullptr;
		std::fill(_key_states.begin(), _key_states.end(), false);
	}

	void editor_first_person_controller::on_window_event(const window_event& ev)
	{
		if (!_world || _entity.is_null())
			return;

		const uint16 button = ev.button;

		switch (ev.type)
		{
		case window_event_type::key: {
			if (button < _key_states.size())
			{
				if (ev.sub_type == window_event_sub_type::press || ev.sub_type == window_event_sub_type::repeat)
					_key_states[button] = true;
				else if (ev.sub_type == window_event_sub_type::release)
					_key_states[button] = false;
			}
			break;
		}
		case window_event_type::mouse: {
			if (button == static_cast<uint16>(input_code::Mouse1))
			{
				const bool was_looking = _is_looking;

				if (ev.sub_type == window_event_sub_type::press || ev.sub_type == window_event_sub_type::repeat)
					_is_looking = true;
				else if (ev.sub_type == window_event_sub_type::release)
				{
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

		_yaw_degrees += _mouse_delta.x * _mouse_sensitivity;
		_pitch_degrees += _mouse_delta.y * _mouse_sensitivity;
		_pitch_degrees = math::clamp(_pitch_degrees, -89.0f, 89.0f);

		entity_manager& manager = _world->get_entity_manager();
		const quat		new_rot = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		manager.set_entity_rotation_abs(_entity, new_rot);

		_mouse_delta = vector2::zero;
	}

	void editor_first_person_controller::apply_movement(float dt_seconds)
	{
		if (!_world || _entity.is_null())
			return;

		auto is_down = [this](uint16 code) -> bool { return code < _key_states.size() ? _key_states[code] : false; };

		entity_manager& manager = _world->get_entity_manager();
		const quat&		rot		= manager.get_entity_rotation_abs(_entity);

		vector3 move_dir = vector3::zero;

		const vector3 forward = rot.get_forward();
		const vector3 right	  = rot.get_right();
		const vector3 up	  = vector3::up;

		if (is_down(static_cast<uint16>(input_code::KeyW)))
			move_dir += forward;
		if (is_down(static_cast<uint16>(input_code::KeyS)))
			move_dir -= forward;
		if (is_down(static_cast<uint16>(input_code::KeyD)))
			move_dir += right;
		if (is_down(static_cast<uint16>(input_code::KeyA)))
			move_dir -= right;
		if (is_down(static_cast<uint16>(input_code::KeyE)) || is_down(static_cast<uint16>(input_code::KeySpace)))
			move_dir += up;
		if (is_down(static_cast<uint16>(input_code::KeyQ)) || is_down(static_cast<uint16>(input_code::KeyLCTRL)) || is_down(static_cast<uint16>(input_code::KeyRCTRL)))
			move_dir -= up;

		if (move_dir.is_zero())
			return;

		move_dir.normalize();

		float move_speed = _move_speed;
		if (is_down(static_cast<uint16>(input_code::KeyLSHIFT)) || is_down(static_cast<uint16>(input_code::KeyRSHIFT)))
			move_speed *= _boost_multiplier;

		vector3 position = manager.get_entity_position_abs(_entity);
		position += move_dir * (move_speed * dt_seconds);
		manager.set_entity_position_abs(_entity, position);
	}
}
