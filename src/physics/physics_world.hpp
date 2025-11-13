// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"
#include "math/vector3.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "physics/physics_layer_filter.hpp"
#include "physics/physics_object_bp_layer_filter.hpp"
#include "physics/physics_bp_layer_interface.hpp"
#include "physics/physics_material_settings.hpp"

#include "world/world_constants.hpp"
#include "world/world_max_defines.hpp"
#include "resources/common_resources.hpp"

namespace JPH
{
	class PhysicsSystem;
	class TempAllocatorImpl;
	class JobSystem;
	class BodyID;
	class Body;
}

namespace SFG
{
	class world;
	class quat;

	class physics_world
	{
	public:
		physics_world() = delete;
		physics_world(world& w) : _game_world(w) {};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void init_simulation();
		void uninit_simulation();
		void simulate(float rate);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		void	   add_bodies_to_world(JPH::BodyID* body_ids, uint32 count);
		void	   add_body_to_world(JPH::Body* body);
		void	   remove_body_from_world(JPH::Body* body);
		JPH::Body* create_body(physics_body_type body_type, physics_shape_type shape, const vector3& extents_or_height_radius, resource_handle physical_material, const vector3& pos, const quat& rot, const vector3& scale);
		void	   destroy_body(JPH::Body* body);
		void	   set_gravity(const vector3& g);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline JPH::PhysicsSystem* get_system() const
		{
			return _system;
		}

		inline world& get_game_world()
		{
			return _game_world;
		}

		inline physical_material_settings& get_default_material()
		{
			return _default_material;
		}

	private:
		JPH::PhysicsSystem*		_system		= nullptr;
		JPH::TempAllocatorImpl* _allocator	= nullptr;
		JPH::JobSystem*			_job_system = nullptr;

		vector<uint32>			   _added_bodies = {};
		world&					   _game_world;
		physical_material_settings _default_material = {};
		vector3					   _graivty			 = vector3::zero;

		physics_layer_filter		   _layer_filter = {};
		physics_object_bp_layer_filter _object_bp_layer_filter;
		physics_bp_layer_interface	   _bp_layer_interface;
	};
}
