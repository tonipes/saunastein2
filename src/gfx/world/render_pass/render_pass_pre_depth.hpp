// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "world/world_max_defines.hpp"

namespace SFG
{
	struct view;
	class proxy_manager;
	struct vector2ui16;
	class renderable_collector;

	class render_pass_pre_depth
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			buffer	  ubo					  = {};
			gfx_id	  cmd_buffer			  = 0;
			gfx_id	  depth_texture			  = 0;
			gpu_index gpu_index_depth_texture = 0;
			gpu_index gpu_index_entity_buffer = 0;
			gpu_index gpu_index_bone_buffer	  = 0;
		};

	public:
		struct init_params
		{
			const vector2ui16& size;
			uint8*			   alloc;
			size_t			   alloc_size;
			gpu_index*		   entities;
			gpu_index*		   bones;
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
		void resize(const vector2ui16& size);

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline gpu_index get_output_gpu_index(uint8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_depth_texture;
		}

		inline gfx_id get_output_hw(uint8 frame_index) const
		{
			return _pfd[frame_index].depth_texture;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		static_vector<indexed_draw, MAX_WORLD_DRAW_CALLS> _draws;
		per_frame_data									  _pfd[BACK_BUFFER_COUNT];
		bump_allocator									  _alloc = {};
	};
}
