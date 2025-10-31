// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"
#include "common/string_id.hpp"
#include <limits>

namespace SFG
{

#define DUMMY_COLOR_TEXTURE_SID	 UINT64_MAX - 1000
#define DUMMY_NORMAL_TEXTURE_SID UINT64_MAX - 999
#define DUMMY_ORM_TEXTURE_SID	 UINT64_MAX - 998
#define DUMMY_SAMPLER_SID		 UINT64_MAX - 997

	typedef pool_handle16 resource_handle;
	typedef uint16		  resource_id;

#define NULL_RESOURCE_ID std::numeric_limits<resource_id>::max();

	enum material_flags : uint32
	{
		material_flags_is_gbuffer	   = 1 << 0,
		material_flags_is_alpha_cutoff = 1 << 1,
		material_flags_is_forward	   = 1 << 2,
		material_flags_is_double_sided = 1 << 3,
	};

	enum shader_variant_flags : uint32
	{
		variant_flag_skinned	  = 1 << 0,
		variant_flag_alpha_cutoff = 1 << 1,
		variant_flag_z_prepass	  = 1 << 2,
		variant_flag_double_sided = 1 << 3,
		variant_flag_shadow_rendering	  = 1 << 4,
	};

}
