// Copyright (c) 2025 Inan Evin
#include "comp_animation_controller.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "resources/audio.hpp"
#include "io/log.hpp"
#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	void comp_animation_controller::on_add(world& w)
	{
		_states = new pool_allocator_gen<animation_state, uint16, 16>();
	}

	void comp_animation_controller::on_remove(world& w)
	{
		delete _states;
		_states = nullptr;
	}

	void comp_animation_controller::serialize(ostream& stream, world& w) const
	{
	}

	void comp_animation_controller::deserialize(istream& stream, world& w)
	{
	}

	void comp_animation_controller::tick(float dt)
	{
		if (!_target_state.is_null())
		{
			const float target_ratio  = _transition_counter / _target_transition;
			const float current_ratio = 1.0f - target_ratio;


			_transition_counter += dt;
			if (_transition_counter >= _target_transition)
			{
				switch_to_state_immediate(_target_state);
			}

			return;
		}
	}

	pool_handle16 comp_animation_controller::add_state()
	{
		return _states->add();
	}

	bool comp_animation_controller::is_valid(pool_handle16 state)
	{
		return _states->is_valid(state);
	}

	void comp_animation_controller::remove_state(pool_handle16 state)
	{
	}

	void comp_animation_controller::switch_to_state(pool_handle16 state, float transition_duration)
	{
		if (_active_state.is_null())
		{
			switch_to_state_immediate(state);
			return;
		}

		_target_state		= state;
		_transition_counter = 0.0f;
		_target_transition	= transition_duration;
	}

	void comp_animation_controller::switch_to_state_immediate(pool_handle16 state)
	{
		_target_state		= {};
		_active_state		= state;
		_target_transition	= 0.0f;
		_transition_counter = 0.0f;
	}
}