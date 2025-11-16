// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "memory/bump_allocator.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;

	class render_pass_bloom
	{
	private:
		static constexpr uint32 MIPS_DS = 5;

		struct ubo
		{
			float filter_radius = 0.2f;
			float pad[3];
		};

		struct per_frame_data
		{
			buffer	  ubo			 = {};
			gfx_id	  cmd_buffer	 = 0;
			gfx_id	  downsample_out = 0;
			gfx_id	  upsample_out	 = 0;
			gpu_index gpu_index_downsample_uav[MIPS_DS];
			gpu_index gpu_index_downsample_srv[MIPS_DS];
			gpu_index gpu_index_upsample_uav[MIPS_DS];
			gpu_index gpu_index_upsample_srv[MIPS_DS];
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   lighting;
			gpu_index		   gpu_index_lighting;
			gfx_id			   global_layout_compute;
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

		void prepare(uint8 frame_index);
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
			return _pfd[frame_index].gpu_index_upsample_srv[0];
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_bloom_downsample = 0;
		gfx_id		   _shader_bloom_upsample	= 0;
		bump_allocator _alloc					= {};
	};
}
