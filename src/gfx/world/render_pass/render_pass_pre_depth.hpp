// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
namespace SFG
{
	struct view;
	class proxy_manager;
	struct vector2ui16;
	struct world_render_data;

	class render_pass_pre_depth
	{
	private:
		static constexpr uint32 MAX_DRAWS = 512;

		struct ubo
		{
			matrix4x4 view		= matrix4x4::identity;
			matrix4x4 proj		= matrix4x4::identity;
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			buffer		   ubo;
			semaphore_data semaphore;
			gfx_id		   cmd_buffer;
			gfx_id		   bind_group;
			gfx_id		   depth_texture = 0;
		};

		struct render_data
		{
			static_vector<indexed_draw, MAX_DRAWS> draws;
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
			uint8			   frame_index;
			world_render_data& wrd;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};
		void init(const init_params& params);
		void uninit();

		void prepare(proxy_manager& pm, view& camera_view, uint8 frame_index, world_render_data& rd);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		inline semaphore_data& get_semaphore(uint8 frame_index)
		{
			return _pfd[frame_index].semaphore;
		}

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline gfx_id get_depth_texture(uint8 frame_index) const
		{
			return _pfd[frame_index].depth_texture;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		render_data	   _render_data;
		bump_allocator _alloc = {};
	};
}
