// Copyright (c) 2025 Inan Evin

#pragma once

namespace SFG
{

	struct indexed_draw
	{
		uint32	  entity_idx		 = 0;
		uint32	  base_vertex		 = 0;
		uint32	  index_count		 = 0;
		uint32	  instance_count	 = 0;
		uint32	  start_index		 = 0;
		uint32	  start_instance	 = 0;
		gpu_index gpu_index_material = 0;
		gpu_index gpu_index_textures = 0;
		gfx_id	  pipeline			 = 0;
		gfx_id	  vertex_buffer		 = 0;
		gfx_id	  idx_buffer		 = 0;

		bool operator==(const indexed_draw& other)
		{
			return vertex_buffer == other.vertex_buffer && idx_buffer == other.idx_buffer && base_vertex == other.base_vertex && index_count == other.index_count && start_index == other.start_index && start_instance == other.start_instance &&
				   pipeline == other.pipeline && gpu_index_material == other.gpu_index_material && gpu_index_textures == other.gpu_index_textures;
		}
	};

}
