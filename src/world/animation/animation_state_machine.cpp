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

#include "animation_state_machine.hpp"

namespace SFG
{
	/*
	void animation_state_machine::tick(float dt)
	{
		if (_active_state.is_null())
			return;

		animation_state& state = _states.get(_active_state);

		for (auto it = _transitions.handles_begin(); it != _transitions.handles_end(); ++it)
		{
			const pool_handle16			transition_handle = *it;
			const animation_transition& t				  = _transitions.get(transition_handle);

			if (t.from_state != _active_state)
				continue;

			if (check_transition(t))
			{
				// cancel active if necessary
				if (!_active_transition.is_null())
				{
					animation_transition& active = _transitions.get(_active_transition);
					if (active.priority <= t.priority)
						reset_transition(active);
				}

				_active_transition = transition_handle;
			}
		}

		// we have an ongoing transition
		if (!_active_transition.is_null())
		{
			animation_transition& active = _transitions.get(_active_transition);

			const float to_ratio   = active.current_time / active.duration;
			const float from_ratio = 1.0f - to_ratio;

			const float new_ratio = progress_transition(active, dt);
			if (math::almost_equal(new_ratio, 1.0f))
			{
				reset_transition(active);
				switch_active_state(active.to_state);
				_active_transition = {};
			}
		}
		else
		{
			animation_state& active = _states.get(_active_state);
		}
	}

	void animation_state_machine::execute_state(world& w, pool_handle16 state_handle, float dt)
	{
		resource_manager&  rm	   = w.get_resource_manager();
		chunk_allocator32& res_aux = rm.get_aux();

		animation_state& state = _states.get(state_handle);

		const uint32		 anims_size = static_cast<uint32>(state.blend_samples.size());
		const animation_mask mask		= state.mask.is_null() ? animation_mask() : get_mask(state.mask);

		animation_pose final_pose = {};

		bool  init	  = false;
		float total_w = 0.0f;

		float weights[MAX_WORLD_BLEND_STATE_ANIMS] = {0.0f};
		compute_state_weights_2d(state, weights, static_cast<uint16>(state.blend_samples.size()));

		for (uint32 i = 0; i < anims_size; i++)
		{
			const animation_blend_sample& sample	  = state.blend_samples[i];
			const resource_handle		  anim_handle = sample.animation;
			const float					  wi		  = weights[i];

			animation_pose pose_i = {};
			pose_i.sample_from_animation(w, anim_handle, state.current_time, mask);

			if (!init)
			{
				total_w	   = wi;
				final_pose = pose_i;
				init	   = true;
			}
			else
			{
				const float t = wi / (total_w + wi);
				final_pose.blend_from(pose_i, t);
				total_w += wi;
			}
		}

		state.current_time = math::clamp(state.current_time + dt, 0.0f, state.duration);
		if (state.flags.is_set(animation_state_flags_is_looping) && state.duration > 0.0f)
			state.current_time = math::fmodf(state.current_time, state.duration);
		else
			state.current_time = math::clamp(state.current_time, 0.0f, state.duration);
	}
	*/

}