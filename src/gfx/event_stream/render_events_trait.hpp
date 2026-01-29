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

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "data/static_vector.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector4ui16.hpp"
#include "game/game_max_defines.hpp"
#include "world/world_constants.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{

	class ostream;
	class istream;

	struct render_event_mesh_instance
	{
		vector<world_id>	skin_node_entities;
		vector<resource_id> materials;
		world_id			entity_index = 0;
		resource_id			mesh		 = 0;
		resource_id			skin		 = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_mesh_instance_material
	{
		resource_id material = NULL_RESOURCE_ID;
		uint32		index	 = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_camera
	{
		static_vector<float, MAX_SHADOW_CASCADES> cascades;
		world_id								  entity_index = 0;
		float									  near_plane   = 0.0f;
		float									  far_plane	   = 0.0f;
		float									  fov_degrees  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_ambient
	{
		vector3	 base_color	  = vector3::one;
		world_id entity_index = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_bloom
	{
		float	 filter_radius = 0.01f;
		world_id entity_index  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_ssao
	{
		float	 radius_world		 = 0.75f;
		float	 bias				 = 0.04f;
		float	 intensity			 = 1.25f;
		float	 power				 = 1.25f;
		uint32	 num_dirs			 = 8;
		uint32	 num_steps			 = 6;
		float	 random_rot_strength = 1.5f;
		world_id entity_index		 = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_post_process
	{
		float	 bloom_strength		  = 0.04f;
		float	 exposure			  = 1.0f;
		int32	 tonemap_mode		  = 1;
		float	 saturation			  = 1.0f;
		float	 wb_temp			  = 0.0f;
		float	 wb_tint			  = 0.0f;
		float	 reinhard_white_point = 6.0f;
		world_id entity_index		  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_skybox
	{
		vector4	 start_color  = vector4(0.2f, 0.1f, 0.2f, 1.0f);
		vector4	 mid_color	  = vector4(0.1f, 0.1f, 0.2f, 1.0f);
		vector4	 end_color	  = vector4(0.2f, 0.1f, 0.1f, 1.0f);
		vector4	 fog_color	  = vector4(0.0f, 0.0f, 0.0f, 0.0f);
		float	 fog_start	  = 0.0f;
		float	 fog_end	  = 0.0f;
		world_id entity_index = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_point_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		range			  = 0.0f;
		float		intensity		  = 0.0f;
		float		near_plane		  = 0.1f;
		bool		cast_shadows	  = false;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_dir_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		intensity		  = 0.0f;
		uint8		cast_shadows	  = 0;
		uint8		max_cascades	  = 1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_spot_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		range			  = 0.0f;
		float		intensity		  = 0.0f;
		float		inner_cone		  = 0.0f;
		float		outer_cone		  = 0.0f;
		float		near_plane		  = 0.1f;
		uint8		cast_shadows	  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_canvas
	{
		world_id entity_index = 0;
		uint32	 vertex_size  = 0;
		uint32	 index_size	  = 0;
		uint8	 is_3d		  = 0;
		void	 serialize(ostream& stream) const;
		void	 deserialize(istream& stream);
	};

	struct render_event_canvas_add_draw
	{
		uint8* vertex_data		= nullptr;
		uint32 vertex_data_size = 0;
		uint8* index_data		= nullptr;
		uint32 index_data_size	= 0;

		vector4ui16 clip;

		uint32		start_index		= 0;
		uint32		index_count		= 0;
		uint32		start_vertex	= 0;
		resource_id material_handle = 0;
		resource_id atlas_handle	= 0;
		uint8		atlas_exists	= 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_particle_emitter
	{
		world_id	entity		 = NULL_WORLD_ID;
		resource_id particle_res = NULL_RESOURCE_ID;
		resource_id material	 = NULL_RESOURCE_ID;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_sprite
	{
		world_id	entity	 = NULL_WORLD_ID;
		resource_id material = NULL_RESOURCE_ID;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
