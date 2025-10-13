// Copyright (c) 2025 Inan Evin

#pragma once
#include "gpu_bone.hpp"
#include "gpu_entity.hpp"
#include "gpu_light.hpp"
#include "renderable.hpp"

namespace SFG
{
#define MAX_RENDERABLES 1024

	struct world_render_data
	{
		static_vector<renderable_object, MAX_RENDERABLES> objects;
		static_vector<gpu_entity, MAX_GPU_ENTITIES>		  entities;
		static_vector<gpu_light, MAX_GPU_LIGHTS>		  lights;
		static_vector<gpu_bone, MAX_GPU_BONES>			  bones;

		inline void reset()
		{
			objects.clear();
			entities.clear();
		}
	};
}
