// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	class proxy_manager;
	struct vector2ui16;
	struct view;
	class renderable_collector;

	class render_pass_lighting
	{
	private:
		struct ubo
		{
			matrix4x4 inverse_view_proj			  = {};
			vector4	  ambient_color_plights_count = vector4::zero;
			vector4	  view_position_slights_count = vector4::zero;
			float	  dir_lights_count			  = 0;
			float	  padding[7]				  = {};
		};

		struct per_frame_data
		{
			buffer	  ubo												 = {};
			gfx_id	  cmd_buffer										 = 0;
			gfx_id	  render_target										 = 0;
			gfx_id	  depth_texture										 = 0;
			gpu_index gpu_index_render_target							 = 0;
			gpu_index gpu_index_depth_texture							 = 0;
			gpu_index gpu_index_gbuffer_textures[GBUFFER_COLOR_TEXTURES] = {NULL_GPU_INDEX};
			gpu_index gpu_index_entity_buffer							 = 0;
			gpu_index gpu_index_point_light_buffer						 = 0;
			gpu_index gpu_index_spot_light_buffer						 = 0;
			gpu_index gpu_index_dir_light_buffer						 = 0;
		};

	public:
		struct init_params
		{
			const vector2ui16& size;
			uint8*			   alloc;
			size_t			   alloc_size;
			gpu_index*		   entities;
			gpu_index*		   point_lights;
			gpu_index*		   spot_lights;
			gpu_index*		   dir_lights;
			gpu_index*		   gbuffer_textures;
			gpu_index*		   depth_textures;
			gfx_id*			   depth_textures_hw;
		};

		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};
		void init(const init_params& params);
		void uninit();

		void prepare(proxy_manager& pm, const renderable_collector& collector, uint8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size, gpu_index* gbuffer_textures, gpu_index* depth_textures, gfx_id* depth_textures_hw);

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
		void create_textures(const vector2ui16& sz, gpu_index* gbuffer_textures, gpu_index* depth_textures, gfx_id* depth_textures_hw);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_lighting = 0;
		bump_allocator _alloc			= {};
	};
}
