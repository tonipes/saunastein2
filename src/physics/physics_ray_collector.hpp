// Copyright (c) 2025 Inan Evin

#pragma once

#include "physics/common_physics.hpp"
#include "physics/physics_ray_result.hpp"
#include "data/vector.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollector.h>

namespace SFG
{
	class physics_world;
	class vector3;

	class physics_raycast_single : public JPH::CollisionCollector<JPH::RayCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::RayCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, float max_distance);

		const ray_result& get_result() const
		{
			return _result;
		}

	private:
		ray_result _result = {};
	};

	class physics_raycast_multi : public JPH::CollisionCollector<JPH::RayCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::RayCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, float max_distance);

		inline const vector<ray_result>& get_results() const
		{
			return _results;
		}

	private:
		vector<JPH::RayCastResult> _jph_results;
		vector<ray_result>		   _results = {};
	};

	class physics_broadphase : public JPH::CollisionCollector<JPH::BroadPhaseCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::BroadPhaseCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, float max_distance);

	private:
		vector<JPH::BroadPhaseCastResult> _jph_results;
		vector<ray_result>				  _results = {};
	};

}
