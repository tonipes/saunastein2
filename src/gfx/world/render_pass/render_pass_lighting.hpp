// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/static_vector.hpp"

// gfx
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;

	class render_pass_lighting
	{
	private:
		struct ubo
		{
			matrix4x4 inverse_view_proj			  = {};
			vector4	  ambient_color_plights_count = vector4::zero;
			vector4	  view_position_slights_count = vector4::zero;
			uint32	  dir_lights_count			  = 0;
			uint32	  cascade_levels_gpu_index	  = 0;
			uint32	  cascade_count				  = 0;
			float	  near_plane				  = 0.0f;
			float	  far_plane					  = 0.0f;
			float	  padding[3]				  = {};
		};

		struct per_frame_data
		{
			buffer	  ubo					  = {};
			gfx_id	  cmd_buffer			  = 0;
			gfx_id	  render_target			  = 0;
			gpu_index gpu_index_render_target = 0;
		};

	public:
		struct render_params
		{
			uint8													frame_index;
			const vector2ui16&										size;
			const static_vector<gpu_index, GBUFFER_COLOR_TEXTURES>& gpu_index_gbuffer_textures;
			gpu_index												gpu_index_depth_texture;
			gpu_index												gpu_index_point_lights;
			gpu_index												gpu_index_spot_lights;
			gpu_index												gpu_index_dir_lights;
			gpu_index												gpu_index_entities;
			gpu_index												gpu_index_shadow_data_buffer;
			gpu_index												gpu_index_float_buffer;
			gfx_id													depth_texture;
			gfx_id													global_layout;
			gfx_id													global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const view& main_camera_view, uint8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(uint8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_render_target;
		}

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_lighting = 0;
		bump_allocator _alloc			= {};
	};
}
