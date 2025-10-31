// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"

#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/draw_stream.hpp"

#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "world/world_max_defines.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct renderable_object;
	class proxy_manager;

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
			gpu_index gpu_index_depth_texture = 0;
			gfx_id	  cmd_buffer			  = 0;
			gfx_id	  depth_texture			  = 0;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
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

		void prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, uint8 frame_index);
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
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		draw_stream	   _draw_stream;
		bump_allocator _alloc = {};
	};
}
