// Copyright (c) 2025 Inan Evin

#include "animation_graph.hpp"
#include "animation_pose.hpp"
#include "math/math.hpp"
#include "world/world.hpp"

namespace SFG
{
	animation_graph::animation_graph()
	{
		_states		 = new states_type();
		_transitions = new transitions_type();
		_params		 = new params_type();
		_masks		 = new masks_type();
		_machines	 = new state_machines_type();
		_samples	 = new state_samples_type();
		_poses		 = new poses_type();
	}

	animation_graph::~animation_graph()
	{
		delete _states;
		delete _transitions;
		delete _params;
		delete _masks;
		delete _machines;
		delete _samples;
		delete _poses;
		_states		 = nullptr;
		_transitions = nullptr;
		_params		 = nullptr;
		_masks		 = nullptr;
		_machines	 = nullptr;
		_samples	 = nullptr;
		_poses		 = nullptr;
	}

	void animation_graph::init()
	{
	}

	void animation_graph::uninit()
	{
		_states->reset();
		_masks->reset();
		_transitions->reset();
		_params->reset();
		_machines->reset();
		_samples->reset();
		_poses->reset();
	}

	void animation_graph::tick(world& w, float dt)
	{
		auto& machines	  = *_machines;
		auto& transitions = *_transitions;
		auto& states	  = *_states;
		auto& samples	  = *_samples;
		auto& poses		  = *_poses;

		animation_pose transition_blend_pose = {};

		uint16 pose_counter = 0;

		for (auto it = machines.handles_begin(); it != machines.handles_end(); ++it)
		{
			const pool_handle16		 machine_handle = *it;
			animation_state_machine& m				= machines.get(machine_handle);

			if (m.active_state.is_null() || m.joint_entities.size == 0)
				continue;

			animation_state& state		= states.get(m.active_state);
			animation_pose&	 final_pose = poses.get(pose_counter);
			pose_counter++;

			// get state's pose.
			final_pose.reset();
			calculate_pose_for_state(w, samples, final_pose, state);
			progress_state(state, dt);

			pool_handle16 existing_transition_handle = m._active_transition;
			pool_handle16 target_transition_handle	 = state._first_out_transition;

			while (!target_transition_handle.is_null())
			{
				animation_transition& t = transitions.get(target_transition_handle);

				// transition fails, switch to next one.
				if (!check_transition(t))
				{
					target_transition_handle = t._next_transition;
					continue;
				}

				// transition passed, and no on-going transitions alive, make this on-going, switch  to next one.
				if (existing_transition_handle.is_null())
				{
					existing_transition_handle = target_transition_handle;
					target_transition_handle   = t._next_transition;
					continue;
				}

				// we have an on-going transition, and also current one pass, check if current one is less important, if so, continue to next one.
				animation_transition& existing = transitions.get(existing_transition_handle);
				if (t.priority < existing.priority)
				{
					target_transition_handle = t._next_transition;
					continue;
				}

				// we have an on-going transition, but current one is more important, reset on-going & switch to this
				reset_transition(existing);
				existing_transition_handle = target_transition_handle;
				target_transition_handle   = t._next_transition;
			}

			m._active_transition = existing_transition_handle;
			if (m._active_transition.is_null())
			{
				continue;
			}

			// Sample the target state of the active transition and blend into the current final pose using transition percentage.
			animation_transition& active	   = transitions.get(m._active_transition);
			animation_state&	  target_state = states.get(active.to_state);
			const float			  ratio		   = progress_transition(active, dt);

			calculate_pose_for_state(w, samples, transition_blend_pose, target_state);
			progress_state(target_state, dt);
			final_pose.blend_from(transition_blend_pose, 1.0f - ratio);

			// reset transition if complete & switch state
			if (math::almost_equal(ratio, 1.0f, 0.001f))
			{
				m._active_transition = {};
				m.active_state		 = active.to_state;
				set_machine_active_state(machine_handle, active.to_state);
				reset_transition(active);
			}
		}

		uint16 i = 0;

		chunk_allocator32& aux = w.get_comp_manager().get_aux();
		entity_manager&	   em  = w.get_entity_manager();

		// now apply generated poses
		for (auto it = machines.handles_begin(); it != machines.handles_end(); ++it)
		{
			const pool_handle16		 machine_handle = *it;
			animation_state_machine& m				= machines.get(machine_handle);

			if (m.active_state.is_null() || m.joint_entities.size == 0)
				continue;

			animation_pose& p = poses.get(i);
			i++;

			const uint16  sz			 = m.joint_entities_count;
			world_handle* entity_handles = aux.get<world_handle>(m.joint_entities);

			const auto& joint_poses = p.get_joint_poses();
			for (const joint_pose& jp : joint_poses)
			{
				const world_handle entity = entity_handles[jp.node_index];

				if (jp.flags.is_set(joint_pose_flags::has_position))
					em.set_entity_position(entity, jp.pos);

				if (jp.flags.is_set(joint_pose_flags::has_rotation))
					em.set_entity_rotation(entity, jp.rot);

				if (jp.flags.is_set(joint_pose_flags::has_scale))
					em.set_entity_scale(entity, jp.scale);
			}
		}
	}

	// -----------------------------------------------------------------------------
	// state/transition/parameter management
	// -----------------------------------------------------------------------------

	pool_handle16 animation_graph::add_state_machine()
	{
		return _machines->add();
	}

	pool_handle16 animation_graph::add_state(pool_handle16 state_machine_handle)
	{
		const pool_handle16 new_state = _states->add();

		animation_state_machine& machine = _machines->get(state_machine_handle);
		if (machine._first_state.is_null())
		{
			machine._first_state = new_state;
			return new_state;
		}

		pool_handle16 next = machine._first_state;
		while (true)
		{
			animation_state&	target = _states->get(next);
			const pool_handle16 h	   = target._next_state;
			if (!h.is_null())
			{
				next = h;
				continue;
			}

			target._next_state = new_state;
			break;
		}

		return new_state;
	}

	pool_handle16 animation_graph::add_state_sample(pool_handle16 state_handle)
	{
		const pool_handle16 new_sample = _samples->add();

		animation_state& state = _states->get(state_handle);

		if (state._first_sample.is_null())
		{
			state._first_sample = new_sample;
			return new_sample;
		}

		pool_handle16 next = state._first_sample;
		while (true)
		{
			animation_state_sample& sample = get_sample(next);
			const pool_handle16		h	   = sample._next_sample;
			if (!h.is_null())
			{
				next = h;
				continue;
			}

			sample._next_sample = new_sample;
			break;
		}

		return new_sample;
	}

	pool_handle16 animation_graph::add_parameter(pool_handle16 state_machine_handle)
	{
		const pool_handle16 new_param = _params->add();

		animation_state_machine& machine = _machines->get(state_machine_handle);
		if (machine._first_parameter.is_null())
		{
			machine._first_parameter = new_param;
			return new_param;
		}

		pool_handle16 next = machine._first_parameter;
		while (true)
		{
			animation_parameter& target = _params->get(next);
			const pool_handle16	 h		= target._next_param;
			if (!h.is_null())
			{
				next = h;
				continue;
			}
			target._next_param = new_param;
			break;
		}

		return new_param;
	}

	pool_handle16 animation_graph::add_transition_from_state(pool_handle16 state_handle)
	{
		const pool_handle16 transition = _transitions->add();

		// add if first transition out of the state
		animation_state& state = _states->get(state_handle);
		pool_handle16	 next  = state._first_out_transition;
		if (next.is_null())
		{
			state._first_out_transition = transition;
			return transition;
		}

		// if not add into the chain.
		while (true)
		{
			animation_transition& target = _transitions->get(next);
			const pool_handle16	  h		 = target._next_transition;
			if (!h.is_null())
			{
				next = h;
				continue;
			}

			target._next_transition = transition;
			break;
		}

		return transition;
	}

	pool_handle16 animation_graph::add_mask()
	{
		return _masks->add();
	}

	void animation_graph::remove_state_machine(pool_handle16 handle)
	{
		animation_state_machine& machine = _machines->get(handle);

		// clear all parameters
		pool_handle16 target_param = machine._first_parameter;
		while (!target_param.is_null())
		{
			animation_parameter& param		= get_parameter(target_param);
			const pool_handle16	 next_param = param._next_param;
			_params->remove(target_param);
			target_param = next_param;
		}

		// clear all states
		pool_handle16 target_state = machine._first_state;
		while (!target_state.is_null())
		{
			animation_state&	state	   = get_state(target_state);
			const pool_handle16 next_state = state._next_state;

			// clear all state transitions
			pool_handle16 target_transition = state._first_out_transition;
			while (!target_transition.is_null())
			{
				animation_transition& transition	  = get_transition(target_transition);
				const pool_handle16	  next_transition = transition._next_transition;
				_transitions->remove(target_transition);
				target_transition = next_transition;
			}

			// clear all state samples
			pool_handle16 target_sample = state._first_sample;
			while (!target_sample.is_null())
			{
				animation_state_sample& sample		= get_sample(target_sample);
				const pool_handle16		next_sample = sample._next_sample;
				_samples->remove(target_sample);
				target_sample = next_sample;
			}

			_states->remove(target_state);
			target_state = next_state;
		}

		_machines->remove(handle);
	}

	void animation_graph::remove_mask(pool_handle16 handle)
	{
		_masks->remove(handle);
	}

	animation_state& animation_graph::get_state(pool_handle16 handle)
	{
		return _states->get(handle);
	}

	animation_state_machine& animation_graph::get_state_machine(pool_handle16 handle)
	{
		return _machines->get(handle);
	}

	animation_transition& animation_graph::get_transition(pool_handle16 handle)
	{
		return _transitions->get(handle);
	}

	animation_parameter& animation_graph::get_parameter(pool_handle16 handle)
	{
		return _params->get(handle);
	}

	animation_mask& animation_graph::get_mask(pool_handle16 handle)
	{
		return _masks->get(handle);
	}

	animation_state_sample& animation_graph::get_sample(pool_handle16 handle)
	{
		return _samples->get(handle);
	}

	void animation_graph::set_machine_active_state(pool_handle16 machine, pool_handle16 state)
	{
		animation_state_machine& m = _machines->get(machine);

		// reset state and it's transitions
		if (!m.active_state.is_null())
		{
			animation_state& active = _states->get(m.active_state);

			pool_handle16 transition = active._first_out_transition;
			while (!transition.is_null())
			{
				animation_transition& t = _transitions->get(transition);
				reset_transition(t);
				transition = t._next_transition;
			}

			reset_state(active);
		}

		m.active_state = state;
	}

	void animation_graph::calculate_pose_for_state(world& w, const state_samples_type& samples, animation_pose& out_pose, animation_state& state)
	{
		resource_manager&  rm	   = w.get_resource_manager();
		chunk_allocator32& res_aux = rm.get_aux();

		static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS> state_animations = {};
		static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>			state_weights	 = {};

		if (state.flags.is_set(animation_state_flags_is_1d))
		{
			compute_state_weights_1d(state, state_animations, state_weights);
		}
		else if (state.flags.is_set(animation_state_flags_is_2d))
		{
			compute_state_weights_2d(state, state_animations, state_weights);
		}
		else
		{
			state_animations.push_back(_samples->get(state._first_sample).animation);
			state_weights.push_back(1.0f);
		}

		const animation_mask mask = state.mask.is_null() ? animation_mask() : get_mask(state.mask);

		bool  init	  = false;
		float total_w = 0.0f;

		const uint16 anims_size = static_cast<uint16>(state_animations.size());

		for (uint16 i = 0; i < anims_size; i++)
		{
			const resource_handle anim_handle = state_animations[i];
			const float			  wi		  = state_weights[i];

			if (math::almost_equal(wi, 0.0f))
				continue;

			animation_pose pose_i = {};
			pose_i.sample_from_animation(w, anim_handle, state._current_time, mask);

			if (!init)
			{
				total_w	 = wi;
				out_pose = pose_i;
				init	 = true;
			}
			else
			{
				const float t = wi / (total_w + wi);
				out_pose.blend_from(pose_i, t);
				total_w += wi;
			}
		}
	}

	void animation_graph::progress_state(animation_state& state, float dt)
	{
		if (state.flags.is_set(animation_state_flags_is_looping) && state.duration > 0.0f)
		{
			state._current_time = math::fmodf(state._current_time + dt, state.duration);
			return;
		}
		state._current_time = math::clamp(state._current_time + dt, 0.0f, state.duration);
	}

	void animation_graph::reset_state(animation_state& state)
	{
		state._current_time = 0.0f;
	}

	void animation_graph::compute_state_weights_1d(animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights)
	{
		const vector2	blend_point = vector2(get_parameter(state.blend_weight_param_x).value, get_parameter(state.blend_weight_param_y).value);
		constexpr float epsilon		= 1e-10f;
		constexpr float epsilon2	= epsilon * epsilon;
		const float		power		= 2.0f;
		uint16			exact_index = UINT16_MAX;

		pool_handle16 sample_handle = state._first_sample;
		SFG_ASSERT(!sample_handle.is_null());

		float weights_sum = 0.0f;

		while (!sample_handle.is_null() && !out_anims.full())
		{
			const animation_state_sample& smp	= _samples->get(sample_handle);
			const float					  d		= blend_point.x - smp.blend_point.x;
			const float					  dist2 = d * d;

			if (dist2 < epsilon2)
			{
				out_anims.clear();
				out_weights.clear();
				out_anims.push_back(smp.animation);
				out_weights.push_back(1.0f);
				return;
			}

			const float dist   = math::sqrt(dist2);
			const float denom  = dist + epsilon;
			const float weight = 1.0f / (denom * denom);
			weights_sum += weight;
			out_weights.push_back(weight);
			out_anims.push_back(smp.animation);
			sample_handle = smp._next_sample;
		}

		const uint16 sz		 = static_cast<uint16>(out_weights.size());
		const float	 inv_sum = 1.0f / weights_sum;
		for (uint16 i = 0; i < sz; ++i)
			out_weights[i] *= inv_sum;
	}

	void animation_graph::compute_state_weights_2d(animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights)
	{
		const vector2	blend_point = vector2(get_parameter(state.blend_weight_param_x).value, get_parameter(state.blend_weight_param_y).value);
		constexpr float epsilon		= 1e-10f;
		constexpr float epsilon2	= epsilon * epsilon;
		const float		power		= 2.0f;
		uint16			exact_index = UINT16_MAX;

		pool_handle16 sample_handle = state._first_sample;
		SFG_ASSERT(!sample_handle.is_null());

		float weights_sum = 0.0f;

		while (!sample_handle.is_null() && !out_anims.full())
		{
			const animation_state_sample& smp	= _samples->get(sample_handle);
			const vector2				  d		= blend_point - smp.blend_point;
			const float					  dist2 = vector2::dot(d, d);

			if (dist2 < epsilon2)
			{
				out_anims.clear();
				out_weights.clear();
				out_anims.push_back(smp.animation);
				out_weights.push_back(1.0f);
				return;
			}

			const float dist   = math::sqrt(dist2);
			const float denom  = dist + epsilon;
			const float weight = 1.0f / (denom * denom);
			weights_sum += weight;
			out_weights.push_back(weight);
			out_anims.push_back(smp.animation);
			sample_handle = smp._next_sample;
		}

		const uint16 sz		 = static_cast<uint16>(out_weights.size());
		const float	 inv_sum = 1.0f / weights_sum;
		for (uint16 i = 0; i < sz; ++i)
			out_weights[i] *= inv_sum;
	}

	// -----------------------------------------------------------------------------
	// transitions
	// -----------------------------------------------------------------------------

	bool animation_graph::check_transition(const animation_transition& t)
	{
		const float value = get_parameter(t.parameter).value;

		switch (t.compare)
		{
		case animation_transition_compare::equals:
			return math::almost_equal(value, t.target_value);
		case animation_transition_compare::not_equals:
			return !math::almost_equal(value, t.target_value);
		case animation_transition_compare::greater:
			return value > t.target_value;
		case animation_transition_compare::lesser:
			return value < t.target_value;
		case animation_transition_compare::gequals:
			return value >= t.target_value;
		case animation_transition_compare::lequals:
			return value <= t.target_value;
		}
		return false;
	}

	bool animation_graph::is_transition_complete(const animation_transition& t)
	{
		const float ratio = t._current_time / t.duration;
		return math::almost_equal(ratio, 1.0f);
	}

	void animation_graph::reset_transition(animation_transition& t)
	{
		t._current_time = 0.0f;
	}

	float animation_graph::progress_transition(animation_transition& t, float dt)
	{
		t._current_time = math::clamp(t._current_time + dt, 0.0f, t.duration);
		return t._current_time / t.duration;
	}
}