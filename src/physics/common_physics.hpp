// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace SFG
{
	enum class physics_broadphase_layers : uint8
	{
		non_moving = 0,
		moving,
	};

	enum class physics_object_layers : uint16
	{
		non_moving = 0,
		moving,
	};

	static constexpr uint16				  PHYSICS_NUM_BP_LAYERS(2);
	static constexpr uint16				  PHYSICS_NUM_OBJ_LAYERS(2);
	static constexpr JPH::BroadPhaseLayer PHYSICS_BP_LAYERS[PHYSICS_NUM_BP_LAYERS] = {};

	enum class physics_body_type : uint8
	{
		static_body,
		kinematic_body,
		dynamic_body,
	};

	enum class physics_shape_type : uint8
	{
		sphere,
		box,
		capsule,
		cylinder,
		plane,
	};

}
