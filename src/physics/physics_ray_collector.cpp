// Copyright (c) 2025 Inan Evin

#include "physics_ray_collector.hpp"
#include "physics/physics_convert.hpp"
#include "world/world.hpp"
#include "physics_world.hpp"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace SFG
{
	void physics_raycast_single::AddHit(const JPH::RayCastResult& inResult)
	{
	}

	bool physics_raycast_single::cast(physics_world* world, const vector3& position, const vector3& normal, float maxDistance)
	{
		_result						 = {};
		const vector3	   direction = normal * maxDistance;
		JPH::RRayCast	   ray(to_jph_vec3(position), to_jph_vec3(direction));
		JPH::RayCastResult ioHit;
		const bool		   hit = world->get_system()->GetNarrowPhaseQuery().CastRay(ray, ioHit);

		if (hit)
		{
			_result.hit_point	 = position + normal * ioHit.mFraction;
			_result.hit_entity	 = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0);
			_result.hit_distance = ioHit.mFraction * normal.magnitude();
		}

		return hit;
	}

	void physics_raycast_multi::AddHit(const JPH::RayCastResult& res)
	{
		_jph_results.push_back(res);
	}

	bool physics_raycast_multi::cast(physics_world* world, const vector3& position, const vector3& normal, float max_distance)
	{
		_jph_results.resize(0);
		_results.resize(0);

		const vector3 direction = normal * max_distance;
		JPH::RRayCast ray(to_jph_vec3(position), to_jph_vec3(direction));

		JPH::RayCastSettings settings;
		world->get_system()->GetNarrowPhaseQuery().CastRay(ray, settings, *this);

		_results.reserve(_jph_results.size());

		for (const JPH::RayCastResult& jph_res : _jph_results)
		{
			ray_result res = {
				.hit_entity	  = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0),
				.hit_point	  = position + normal * jph_res.mFraction,
				.hit_distance = jph_res.mFraction * direction.magnitude(),
			};
			_results.push_back(res);
		}

		return _results.empty();
	}

	void physics_broadphase::AddHit(const JPH::BroadPhaseCastResult& res)
	{
		_jph_results.push_back(res);
	}

	bool physics_broadphase::cast(physics_world* world, const vector3& position, const vector3& normal, float max_distance)
	{
		_jph_results.resize(0);
		_results.resize(0);

		const vector3 direction = normal * max_distance;
		JPH::RayCast  ray(to_jph_vec3(position), to_jph_vec3(direction));

		JPH::RayCastSettings settings;
		world->get_system()->GetBroadPhaseQuery().CastRay(ray, *this);

		for (const JPH::BroadPhaseCastResult& jph_res : _jph_results)
		{
			ray_result res = {
				.hit_entity	  = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0),
				.hit_point	  = position + normal * jph_res.mFraction,
				.hit_distance = jph_res.mFraction * direction.magnitude(),
			};
			_results.push_back(res);
		}

		return _results.empty();
	}

}
