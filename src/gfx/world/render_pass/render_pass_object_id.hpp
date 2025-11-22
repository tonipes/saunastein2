// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector2ui16.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct renderable_object;
	class proxy_manager;

	class render_pass_object_id
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			buffer ubo;
			gfx_id render_target   = NULL_GFX_ID;
			gfx_id cmd_buffer	   = 0;
			gfx_id readback_buffer = 0;
			uint8* readback_mapped = nullptr;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
			gfx_id			   depth_texture;
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

		void   prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, uint8 frame_index);
		void   render(const render_params& p);
		void   resize(const vector2ui16& size);
		uint32 read_location(uint16 x, uint16 y, uint8 frame_index);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		draw_stream	   _draw_stream;
		bump_allocator _alloc = {};
		vector2ui16	   _size  = {};
	};
}
