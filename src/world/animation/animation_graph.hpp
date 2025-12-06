// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "animation_state.hpp"
#include "animation_transition.hpp"
#include "animation_mask.hpp"
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
	struct animation_pose;

	class animation_graph
	{

	private:
		using states_type		  = pool_allocator_gen<animation_state, uint16, MAX_WORLD_ANIM_GRAPH_STATES>;
		using poses_type		  = pool_allocator_simple<animation_pose, MAX_WORLD_ANIM_GRAPH_STATES, uint16>;
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
		pool_handle16			 add_state_sample(pool_handle16 state_handle);
		pool_handle16			 add_parameter(pool_handle16 state_machine_handle);
		pool_handle16			 add_transition_from_state(pool_handle16 state_handle);
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

	private:
		// -----------------------------------------------------------------------------
		// states
		// -----------------------------------------------------------------------------
		void calculate_pose_for_state(world& w, const state_samples_type& samples, animation_pose& out_pose, animation_state& state);
		void progress_state(animation_state& state, float dt);
		void reset_state(animation_state& state);

		// -----------------------------------------------------------------------------
		// weights
		// -----------------------------------------------------------------------------

		void compute_state_weights_1d(animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);
		void compute_state_weights_2d(animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<float, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);

		// -----------------------------------------------------------------------------
		// transitions
		// -----------------------------------------------------------------------------

		bool  check_transition(const animation_transition& t);
		bool  is_transition_complete(const animation_transition& t);
		void  reset_transition(animation_transition& t);
		float progress_transition(animation_transition& t, float dt);

	private:
		states_type*		 _states	  = nullptr;
		poses_type*			 _poses		  = nullptr;
		transitions_type*	 _transitions = nullptr;
		params_type*		 _params	  = nullptr;
		masks_type*			 _masks		  = nullptr;
		state_machines_type* _machines	  = nullptr;
		state_samples_type*	 _samples	  = nullptr;
	};
}
