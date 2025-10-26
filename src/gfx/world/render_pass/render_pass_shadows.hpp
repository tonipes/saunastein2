// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "world/world_max_defines.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/matrix4x4.hpp"
#include "data/static_vector.hpp"

namespace SFG
{
	struct view;
	class proxy_manager;
	struct vector2ui16;
	struct view;
	struct world_render_data;

	class render_pass_shadows
	{

	private:
		static constexpr size_t SHADOW_PASSES_COUNT = MAX_SHADOW_CASTING_DIR_LIGHTS * SHADOWS_CASCADES + MAX_SHADOW_CASTING_POINT_LIGHTS * 6 + MAX_SHADOW_CASTING_SPOT_LIGHTS;

		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			gfx_id cmd_buffers[SHADOWS_MAX_CMD_BUFFERS];
			gfx_id bind_group;
			uint8  active_cmd_buffers = 0;
		};

		struct pass
		{
			static_vector<indexed_draw, MAX_WORLD_DRAW_CALLS> draws;
			buffer											  ubos[BACK_BUFFER_COUNT]		 = {};
			gfx_id											  bind_groups[BACK_BUFFER_COUNT] = {};
			renderable_collector							  collector						 = {};
			gfx_id											  texture						 = 0;
		};

	public:
		struct init_params
		{
			const vector2ui16& size;
			uint8*			   alloc;
			size_t			   alloc_size;
			gfx_id*			   entities;
			gfx_id*			   bones;
		};

		struct render_params
		{
			uint8			   cmd_index;
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		void init(const init_params& params);
		void uninit();

		void prepare(proxy_manager& pm, uint8 frame_index);
		void render(const render_params& params);

		inline const gfx_id* get_cmd_buffers(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffers;
		}

		inline uint8 get_active_cmd_buffers_count(uint8 frame_index) const
		{
			return _pfd[frame_index].active_cmd_buffers;
		}

	private:
		void prepare_pass(pass& p);
		void render_passes(gfx_id cmd_buffer, pass* p, uint8 pass_count);

	private:
		per_frame_data							 _pfd[BACK_BUFFER_COUNT];
		bump_allocator							 _alloc	 = {};
		static_vector<pass, SHADOW_PASSES_COUNT> _passes = {};
	};
}
