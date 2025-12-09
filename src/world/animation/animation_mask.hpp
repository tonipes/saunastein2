// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "common/string_id.hpp"
#include "game/game_max_defines.hpp"
#include "data/static_vector.hpp"

namespace SFG
{
	class world;

	class animation_mask
	{
	public:
		struct joint_data
		{
			uint8 is_masked = 0;
		};

		void mask_joints(world& w, resource_handle skin, string_id* name_hashes, uint16 name_hashes_count);
		void mask_joint(world& w, resource_handle skin, string_id hash);

		inline const joint_data* get_mask() const
		{
			return _joints;
		}

		inline void mask_joint(int16 j)
		{
			_joints[j].is_masked = 1;
		}

		inline bool is_masked(int16 j) const
		{
			return _joints[j].is_masked;
		}

		inline void reset()
		{
			for (uint16 i = 0; i < MAX_WORLD_SKELETON_JOINTS; i++)
				_joints[i].is_masked = 0;
		}

	private:
		joint_data _joints[MAX_WORLD_SKELETON_JOINTS];
	};

}
