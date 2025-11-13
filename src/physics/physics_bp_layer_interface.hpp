// Copyright (c) 2025 Inan Evin

#pragma once

#include "physics/common_physics.hpp"
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace SFG
{
	class physics_bp_layer_interface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual ~physics_bp_layer_interface() = default;

		virtual uint32 GetNumBroadPhaseLayers() const override
		{
			return PHYSICS_NUM_BP_LAYERS;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			const physics_object_layers layer = static_cast<physics_object_layers>(inLayer);

			if (layer == physics_object_layers::non_moving)
				return PHYSICS_BP_LAYERS[(uint16)physics_broadphase_layers::non_moving];

			return PHYSICS_BP_LAYERS[(uint16)physics_broadphase_layers::moving];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
	};
}
