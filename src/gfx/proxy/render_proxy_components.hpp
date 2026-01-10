/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "render_proxy_common.hpp"
#include "data/vector.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"

// math
#include "math/color.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector4ui16.hpp"

// gfx
#include "gfx/common/gfx_constants.hpp"

// world
#include "world/world_constants.hpp"
#include "game/game_max_defines.hpp"
#include "gfx/buffer.hpp"

namespace SFG
{
	struct render_proxy_mesh_instance
	{
		chunk_handle32 skin_entities		= {};
		uint32		   _assigned_bone_index = 0;
		world_id	   entity				= 0;
		chunk_handle32 materials			= {};
		resource_id	   mesh					= NULL_RESOURCE_ID;
		resource_id	   skin					= NULL_RESOURCE_ID;
		uint16		   skin_entities_count	= 0;
		uint16		   materials_count		= 0;
	};

	struct render_proxy_camera
	{
		chunk_handle32		cascades	  = {};
		world_id			entity		  = 0;
		float				near_plane	  = 0.0f;
		float				far_plane	  = 0.0f;
		float				fov_degrees	  = 0.0f;
		uint8				cascade_count = 0;
		render_proxy_status status		  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_ambient
	{
		vector3				base_color = vector3::one;
		world_id			entity	   = 0;
		render_proxy_status status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_dir_light
	{
		vector3				base_color									= vector3::one;
		vector2ui16			shadow_res									= vector2ui16(256, 256);
		gfx_id				shadow_texture_hw[BACK_BUFFER_COUNT]		= {NULL_GFX_ID};
		gfx_id				shadow_texture_gpu_index[BACK_BUFFER_COUNT] = {NULL_GFX_ID};
		float				intensity									= 0;
		world_id			entity										= 0;
		uint8				cast_shadows								= 0;
		uint8				cascade_levels								= 0;
		render_proxy_status status										= render_proxy_status::rps_inactive;
	};

	struct render_proxy_point_light
	{
		vector3				base_color									= vector3::one;
		vector2ui16			shadow_res									= vector2ui16(256, 256);
		gfx_id				shadow_texture_hw[BACK_BUFFER_COUNT]		= {NULL_GFX_ID};
		gfx_id				shadow_texture_gpu_index[BACK_BUFFER_COUNT] = {NULL_GFX_ID};
		float				range										= 0.0f;
		float				intensity									= 0;
		float				near_plane									= 0.1f;
		world_id			entity										= 0;
		uint8				cast_shadows								= 0;
		render_proxy_status status										= render_proxy_status::rps_inactive;
	};

	struct render_proxy_spot_light
	{
		vector3				base_color									= vector3::one;
		vector2ui16			shadow_res									= vector2ui16(256, 256);
		gfx_id				shadow_texture_hw[BACK_BUFFER_COUNT]		= {NULL_GFX_ID};
		gfx_id				shadow_texture_gpu_index[BACK_BUFFER_COUNT] = {NULL_GFX_ID};
		float				range										= 0.0f;
		float				intensity									= 0;
		float				inner_cone									= 0.0f;
		float				outer_cone									= 0.0f;
		float				near_plane									= 0.1f;
		world_id			entity										= 0;
		uint8				cast_shadows								= 0;
		render_proxy_status status										= render_proxy_status::rps_inactive;
	};

	struct render_proxy_canvas_dc
	{
		vector4ui16 clip		 = vector4ui16::zero;
		uint32		start_index	 = 0;
		uint32		start_vertex = 0;
		uint32		index_count	 = 0;
		resource_id mat_id		 = 0;
		resource_id atlas_id	 = 0;
		uint8		atlas_exists = 0;
	};

	struct render_proxy_canvas
	{
		buffer_cpu_gpu				   vertex_buffers[BACK_BUFFER_COUNT] = {};
		buffer_cpu_gpu				   index_buffers[BACK_BUFFER_COUNT]	 = {};
		vector<render_proxy_canvas_dc> _draw_calls;
		uint32						   _max_vertex_offset = 0;
		uint32						   _max_index_offset  = 0;
		world_id					   entity			  = 0;
		uint8						   is_3d			  = 0;
		render_proxy_status			   status			  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_particle_emitter
	{
		world_id			entity			= {};
		resource_id			particle_res_id = NULL_RESOURCE_ID;
		resource_id			material		= NULL_RESOURCE_ID;
		float				last_emitted	= 0.0f;
		float				current_life	= 0.0f;
		render_proxy_status status			= render_proxy_status::rps_inactive;
	};
}
