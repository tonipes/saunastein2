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

#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"

namespace SFG
{
	struct view;
	class proxy_manager;

	class render_pass_particles
	{
	private:
		struct indirect_render
		{
			uint32 vertex_count;
			uint32 vertex_start;
			uint32 instance_count;
			uint32 instance_start;
		};

		struct indirect_dispatch
		{
			uint32 group_sim_x;
			uint32 group_sim_y;
			uint32 group_sim_z;
			uint32 group_count_x;
			uint32 group_count_y;
			uint32 group_count_z;
		};

		struct pass_params
		{
			matrix4x4 view_proj;
			vector4	  cam_pos_and_delta;
			vector4	  cam_dir;
			uint32	  max_particles_per_system;
			uint32	  frame_index;
			uint32	  max_systems;
			uint32	  num_systems;
		};

		struct particle_system_data
		{
			uint32 alive_count;
			uint32 dead_count;
		};

		struct particle_emit_args
		{
			vector4 min_color;
			vector4 max_color;
			vector4 min_pos; // w is min lifetime
			vector4 max_pos; // w is max lifetime
			vector4 min_vel; // w is min rotation
			vector4 max_vel; // w is max rotation
		};

		struct particle_state
		{
			vector4 position_and_age;
			vector4 color;
			vector4 velocity_and_lifetime;
			float	rotation;
			uint32	system_id;
			float	padding[2];
		};

		struct particle_instance_data
		{
			vector4 pos_and_rot;
			vector4 size;
		};

		struct particle_counters
		{
			uint32 alive_count_a;
			uint32 alive_count_b;
		};

		struct particle_indirect_args
		{
			uint32 vertex_count;
			uint32 instance_count;
			uint32 start_vertex;
			uint32 start_instance;
		};

		struct particle_sim_count_args
		{
			uint32 group_sim_x;
			uint32 group_sim_y;
			uint32 group_sim_z;
			uint32 group_count_x;
			uint32 group_count_y;
			uint32 group_count_z;
		};

		struct per_frame_data
		{
			buffer ubo				  = {};

			gfx_id cmd_buffer		  = NULL_GFX_ID;
			gfx_id cmd_buffer_compute = NULL_GFX_ID;
		};

	public:
		struct render_params
		{
			uint8	  frame_index;
			gfx_id	  global_layout_compute;
			gfx_id	  global_layout;
			gfx_id	  global_group;
			gpu_index gpu_index_lighting;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(uint8 frame_index, proxy_manager& pm, const view& main_camera_view);
		void compute(uint8 frame_index);
		void render(const render_params& params);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline gfx_id get_cmd_buffer_compute(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer_compute;
		}

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _indirect_render_sig	  = 0;
		gfx_id		   _indirect_dispatch_sig = 0;
		gfx_id		   _shader_clear		  = NULL_GFX_ID;
		gfx_id		   _shader_simulate		  = NULL_GFX_ID;
		gfx_id		   _shader_emit			  = NULL_GFX_ID;
		gfx_id		   _shader_write_count	  = NULL_GFX_ID;
		gfx_id		   _shader_count		  = NULL_GFX_ID;
		gfx_id		   _shader_swap			  = NULL_GFX_ID;
	};
}
