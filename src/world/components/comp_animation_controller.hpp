// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/component_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "world/animation/animation_state.hpp"

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class comp_animation_controller
	{
	public:
		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void		  tick(float dt);
		pool_handle16 add_state();
		bool		  is_valid(pool_handle16 state);
		void		  remove_state(pool_handle16 state);
		void		  switch_to_state(pool_handle16 state, float transition_duration);
		void		  switch_to_state_immediate(pool_handle16 state);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header								 _header			 = {};
		pool_allocator_gen<animation_state, uint16, 16>* _states			 = nullptr;
		pool_handle16									 _active_state		 = {};
		pool_handle16									 _target_state		 = {};
		float											 _transition_counter = 0.0f;
		float											 _target_transition	 = 0.0f;
	};

	REGISTER_TRAIT(comp_animation_controller);
}
