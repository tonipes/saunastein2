// Copyright (c) 2025 Inan Evin

#include "physics_bp_layer_interface.hpp"
#include "io/assert.hpp"
namespace SFG
{
	const char* SFG::physics_bp_layer_interface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)physics_broadphase_layers::non_moving:
				return "NON_MOVING";
			case (JPH::BroadPhaseLayer::Type)physics_broadphase_layers::moving:
				return "MOVING";
			default:
				SFG_ASSERT(false);
				return "INVALID";
			}
		}
	}
}
