// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "animation_state.hpp"
#include "animation_transition.hpp"
#include "animation_mask.hpp"
#include "animation_pose.hpp"
#include "animation_state_machine.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{

	struct animation_parameter
	{
		pool_handle16 _next_param = {};
		float		  value		  = 0.0f;
	};

	class world;

	class animation_graph
	{

	private:
		using states_type		  = pool_allocator_gen<animation_state, uint16, MAX_WORLD_ANIM_GRAPH_STATES>;
		using transitions_type	  = pool_allocator_gen<animation_transition, uint16, MAX_WORLD_ANIM_GRAPH_TRANSITION>;
		using params_type		  = pool_allocator_gen<animation_parameter, uint16, MAX_WORLD_ANIM_GRAPH_PARAMETER>;
		using masks_type		  = pool_allocator_gen<animation_mask, uint16, MAX_WORLD_ANIM_GRAPH_MASK>;
		using state_machines_type = pool_allocator_gen<animation_state_machine, uint16, MAX_WORLD_COMP_ANIMS>;
		using state_samples_type  = pool_allocator_gen<animation_state_sample, uint16, MAX_WORLD_ANIM_GRAPH_STATE_SAMPLES>;

	public:
		animation_graph();
		~animation_graph();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick(world& w, float dt);

		// -----------------------------------------------------------------------------
		// state/transition/parameter management
		// -----------------------------------------------------------------------------

		pool_handle16			 add_state_machine();
		pool_handle16			 add_state(pool_handle16 state_machine_handle);
		pool_handle16			 add_state(pool_handle16 state_machine_handle, pool_handle16 mask, pool_handle16 blend_param_x, pool_handle16 blend_param_y, float duration, uint8 flags);
		pool_handle16			 add_state_sample(pool_handle16 state_handle, resource_handle animation, const vector2& blend_point);
		pool_handle16			 add_state_sample(pool_handle16 state_handle);
		pool_handle16			 add_parameter(pool_handle16 state_machine_handle, float val = 0.0f);
		pool_handle16			 add_transition(pool_handle16 from_state_handle, pool_handle16 to_state_handle);
		pool_handle16			 add_transition(pool_handle16 from_state_handle, pool_handle16 to_state_handle, pool_handle16 param_handle, float duration, float target_value, animation_transition_compare compare, uint8 priority);
		pool_handle16			 add_mask();
		void					 remove_state_machine(pool_handle16 handle);
		void					 remove_mask(pool_handle16 handle);
		animation_state&		 get_state(pool_handle16 handle);
		animation_state_machine& get_state_machine(pool_handle16 handle);
		animation_transition&	 get_transition(pool_handle16 handle);
		animation_parameter&	 get_parameter(pool_handle16 handle);
		animation_mask&			 get_mask(pool_handle16 handle);
		animation_state_sample&	 get_sample(pool_handle16 handle);
		void					 set_machine_active_state(pool_handle16 machine, pool_handle16 state);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_throttle_values(float max_distance_sqr, uint8 max_frames)
		{
			_throttle_distance_sqr = max_distance_sqr;
			_throttle_max_frames   = max_frames;
		}

	private:
		// -----------------------------------------------------------------------------
		// machine
		// -----------------------------------------------------------------------------

		void apply_pose(world& w, const animation_state_machine& m, const animation_pose& pose);

		// -----------------------------------------------------------------------------
		// states
		// -----------------------------------------------------------------------------

		void calculate_pose_for_state(world& w, const state_samples_type& samples, animation_pose& out_pose, const animation_state& state);
		void progress_state(animation_state& state, float dt);
		void reset_state(animation_state& state);

		// -----------------------------------------------------------------------------
		// weights
		// -----------------------------------------------------------------------------

		void compute_state_weights_1d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);
		void compute_state_weights_2d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);

		// -----------------------------------------------------------------------------
		// transitions
		// -----------------------------------------------------------------------------

		bool  check_transition(const animation_transition& t);
		bool  is_transition_complete(const animation_transition& t);
		void  reset_transition(animation_transition& t);
		float progress_transition(animation_transition& t, float dt);

	private:
		states_type*		 _states	  = nullptr;
		transitions_type*	 _transitions = nullptr;
		params_type*		 _params	  = nullptr;
		masks_type*			 _masks		  = nullptr;
		state_machines_type* _machines	  = nullptr;
		state_samples_type*	 _samples	  = nullptr;

		float _throttle_distance_sqr = 500.0f;
		uint8 _throttle_max_frames	 = 4;
	};
}
