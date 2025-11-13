// Copyright (c) 2025 Inan Evin

#pragma once

#include "physics/common_physics.hpp"

namespace SFG
{

	class physics_object_bp_layer_filter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			const physics_object_layers objLayer1 = static_cast<physics_object_layers>(inLayer1);

			if (objLayer1 == physics_object_layers::non_moving)
				return inLayer2 == PHYSICS_BP_LAYERS[(uint16)physics_broadphase_layers::moving];

			return true;
		}
	};

}
