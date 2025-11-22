// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/static_vector.hpp"

// gfx
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "math/vector2.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;

	class render_pass_post_combiner
	{
	private:
		struct ubo
		{
			vector2 screen_size			 = vector2::zero;
			float	bloom_strength		 = 0.0f;
			float	exposure			 = 0;
			int		tonemap_mode		 = 0;
			float	saturation			 = 1.0f;
			float	wb_temp				 = 0.0f;
			float	wb_tint				 = 0.0f;
			float	reinhard_white_point = 3.0f;
			float	pad;
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
			uint8			   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_lighting;
			gpu_index		   gpu_index_bloom;
			gpu_index		   gpu_index_selection_outline;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(uint8 frame_index, const vector2ui16& resolution);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(uint8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_render_target;
		}

		inline gpu_index get_output_hw(uint8 frame_index) const
		{
			return _pfd[frame_index].render_target;
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
		gfx_id		   _shader_post_combiner = 0;
	};
}
