// Copyright (c) 2025 Inan Evin

#pragma once

#include "physics/common_physics.hpp"

namespace SFG
{
	class physics_layer_filter : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			physics_object_layers inLayer1 = static_cast<physics_object_layers>(inObject1);
			physics_object_layers inLayer2 = static_cast<physics_object_layers>(inObject2);

			if (inLayer1 == physics_object_layers::non_moving)
				return inLayer2 == physics_object_layers::moving;

			return true;
		}
	};
}
