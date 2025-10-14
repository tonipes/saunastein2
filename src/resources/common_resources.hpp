// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"
#include "common/string_id.hpp"
#include <limits>
namespace SFG
{

#define MAX_WORLD_MODEL_INSTANCES	 5000
#define MAX_WORLD_TEXTURES			 32
#define MAX_WORLD_MODELS			 32
#define MAX_WORLD_MESHES			 256
#define MAX_WORLD_ANIMS				 32
#define MAX_WORLD_SKINS				 12
#define MAX_WORLD_MATERIALS			 20
#define MAX_WORLD_SHADERS			 30
#define MAX_WORLD_AUDIO				 50
#define MAX_WORLD_FONTS				 20
#define MAX_WORLD_PHYSICAL_MATERIALS 30
#define MAX_WORLD_SAMPLERS			 16

#define DUMMY_COLOR_TEXTURE_SID	 UINT64_MAX - 1000
#define DUMMY_NORMAL_TEXTURE_SID UINT64_MAX - 999
#define DUMMY_ORM_TEXTURE_SID	 UINT64_MAX - 998

	enum resource_type : uint8
	{
		resource_type_texture = 0,
		resource_type_texture_sampler,
		resource_type_model,
		resource_type_animation,
		resource_type_skin,
		resource_type_material,
		resource_type_shader,
		resource_type_audio,
		resource_type_font,
		resource_type_mesh,
		resource_type_physical_material,
		resource_type_max,
	};

	typedef pool_handle16 resource_handle;
	typedef uint16		  resource_id;

#define NULL_RESOURCE_ID std::numeric_limits<resource_id>::max();

	enum material_flags
	{
		material_flags_is_gbuffer = 1 << 0,
		material_flags_is_forward = 1 << 1,
	};

	enum res_shader_flags
	{
		res_shader_flags_is_skinned = 1 << 0,
	};

}
