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
#include "gfx/draw_stream.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"
#include "math/vector2.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
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

		struct ubo
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
			vector4 integrate_points; // vel, op, ang, sz
			vector4 opacity_points;	  // min_start, max_start, mid, end
			vector4 size_points;	  // min_start, max_start, mid, end

			float min_lifetime;
			float max_lifetime;

			float min_pos_x;
			float min_pos_y;
			float min_pos_z;
			float max_pos_x;
			float max_pos_y;
			float max_pos_z;

			float min_start_vel_x;
			float min_start_vel_y;
			float min_start_vel_z;
			float max_start_vel_x;
			float max_start_vel_y;
			float max_start_vel_z;

			float min_mid_vel_x;
			float min_mid_vel_y;
			float min_mid_vel_z;
			float max_mid_vel_x;
			float max_mid_vel_y;
			float max_mid_vel_z;

			float min_end_vel_x;
			float min_end_vel_y;
			float min_end_vel_z;
			float max_end_vel_x;
			float max_end_vel_y;
			float max_end_vel_z;

			float min_col_x;
			float min_col_y;
			float min_col_z;
			float max_col_x;
			float max_col_y;
			float max_col_z;

			float min_start_rotation;
			float max_start_rotation;
			float min_start_angular_velocity;
			float max_start_angular_velocity;
			float min_end_angular_velocity;
			float max_end_angular_velocity;
		};

		struct particle_state
		{
			float pos_x;
			float pos_y;
			float pos_z;

			float age;
			float lifetime;

			float start_vel_x;
			float start_vel_y;
			float start_vel_z;
			float mid_vel_x;
			float mid_vel_y;
			float mid_vel_z;
			float end_vel_x;
			float end_vel_y;
			float end_vel_z;

			float rotation;
			float start_ang_vel;
			float end_ang_vel;

			uint32 start_size_opacity;
			uint32 mid_size_opacity;
			uint32 end_size_opacity;
			uint32 size_opacity_integrate_point;
			uint32 vel_and_ang_vel_integrate_point;

			uint32 color;
			uint32 system_id;
		};

		struct particle_instance_data
		{
			vector4 pos_rot_size;
			uint32	color;
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
			buffer_gpu ubo				  = {};
			buffer	   indirect_arguments = {};
			buffer	   instance_data	  = {};
			gfx_id	   cmd_buffer		  = NULL_GFX_ID;
			gfx_id	   cmd_buffer_compute = NULL_GFX_ID;
		};

		struct sim_state
		{
			buffer emit_counts					= {};
			buffer system_data					= {};
			buffer emit_arguments				= {};
			buffer states						= {};
			buffer sim_count_indirect_arguments = {};
			buffer alive_list_a					= {};
			buffer alive_list_b					= {};
			buffer dead_indices					= {};
			buffer counters						= {};
			uint8  frame_switch					= 0;
			uint8  buffers_init					= 0;
		};

	public:
		struct compute_params
		{
			uint8  frame_index;
			gfx_id global_layout_compute;
			gfx_id global_group;
		};

		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
			gfx_id			   hw_lighting;
			gfx_id			   hw_depth;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(gfx_id bind_layout, gfx_id bind_layout_compute);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(uint8 frame_index, proxy_manager& pm, const view& main_camera_view);
		void compute(const compute_params& p);
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
		per_frame_data		 _pfd[BACK_BUFFER_COUNT];
		sim_state			 _sim_state = {};
		draw_stream_particle _draw_stream;
		bump_allocator		 _alloc					= {};
		uint32				 _num_systems			= 0;
		gfx_id				 _shader_clear			= NULL_GFX_ID;
		gfx_id				 _shader_simulate		= NULL_GFX_ID;
		gfx_id				 _shader_emit			= NULL_GFX_ID;
		gfx_id				 _shader_write_count	= NULL_GFX_ID;
		gfx_id				 _shader_count			= NULL_GFX_ID;
		gfx_id				 _shader_swap			= NULL_GFX_ID;
		gfx_id				 _indirect_sig_dispatch = NULL_GFX_ID;
		gfx_id				 _indirect_sig_draw		= NULL_GFX_ID;
	};
}
