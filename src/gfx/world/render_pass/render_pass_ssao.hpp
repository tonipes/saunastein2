// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/static_vector.hpp"

// gfx
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/vector2ui.hpp"
#include "math/vector2.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;
	class texture_queue;

	class render_pass_ssao
	{
	private:
		struct ubo
		{
			matrix4x4 proj_matrix;
			matrix4x4 inv_proj_matrix;
			matrix4x4 view_matrix;

			vector2ui full_size;
			vector2ui half_size;
			vector2	  inv_full;
			vector2	  inv_half;

			// Projection
			float z_near; // near plane (if you linearize from device depth)
			float z_far;  // far plane  (if you linearize from device depth)

			// AO controls
			float  radius_world;		// AO radius in *world/view* units (meters)
			float  bias;				// small bias to reduce self-occlusion (e.g., 0.02)
			float  intensity;			// AO strength (e.g., 1.2)
			float  power;				// contrast shaping (e.g., 1.1)
			uint32 num_dirs;			// e.g., 8
			uint32 num_steps;			// e.g., 6
			float  random_rot_strength; // e.g., 1.0
		};

		struct per_frame_data
		{
			buffer	  ubo						= {};
			gfx_id	  cmd_buffer				= 0;
			gfx_id	  ao_out					= 0;
			gfx_id	  ao_upsample_out			= 0;
			gpu_index gpu_index_ao_uav			= 0;
			gpu_index gpu_index_ao_srv			= 0;
			gpu_index gpu_index_ao_upsample_uav = 0;
			gpu_index gpu_index_ao_upsample_srv = 0;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_depth;
			gpu_index		   gpu_index_normals;
			gfx_id			   global_layout_compute;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size, texture_queue& tq);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline gpu_index get_output_gpu_index(uint8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_ao_upsample_srv;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_hbao			   = 0;
		gfx_id		   _shader_hbao_upsample   = 0;
		gfx_id		   _noise_tex			   = 0;
		gfx_id		   _noise_tex_intermediate = 0;
		gpu_index	   _gpu_index_noise		   = 0;
		bump_allocator _alloc				   = {};
	};
}
