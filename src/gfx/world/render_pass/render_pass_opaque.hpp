// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "world/world_max_defines.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	class buffer_queue;
	class proxy_manager;
	struct vector2ui16;
	struct view;
	class renderable_collector;

	class render_pass_opaque
	{
	public:
		static constexpr uint32 COLOR_TEXTURES = 4;

	private:

		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			buffer ubo;

			static_vector<gfx_id, COLOR_TEXTURES> color_textures;
			gfx_id								  cmd_buffer	= 0;
			gfx_id								  bind_group	= 0;
			gfx_id								  depth_texture = 0;
		};

		struct render_data
		{
			static_vector<indexed_draw, MAX_WORLD_DRAW_CALLS> draws;
		};

	public:
		struct init_params
		{
			const vector2ui16& size;
			uint8*			   alloc;
			size_t			   alloc_size;
			gfx_id*			   entities;
			gfx_id*			   bones;
			gfx_id*			   depth_textures;
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
		void resize(const vector2ui16& size, gfx_id* depth_textures);

		inline gfx_id get_color_texture(uint8 frame_index, uint8 texture_index) const
		{
			return _pfd[frame_index].color_textures[texture_index];
		}

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline void set_depth_textures(gfx_id* textures)
		{
			for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				_pfd[i].depth_texture = textures[i];
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz, gfx_id* depth_textures);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		render_data	   _render_data;
		bump_allocator _alloc = {};
	};
}
