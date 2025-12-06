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
		void mask_joints(world& w, resource_handle skin, string_id* name_hashes, uint16 name_hashes_count);
		void mask_joint(world& w, resource_handle skin, string_id hash);

		inline const static_vector<int16, MAX_WORLD_SKELETON_JOINTS>& get_mask() const
		{
			return _masked_joints;
		}

		inline void mask_joint(int16 j)
		{
			_masked_joints.push_back(j);
		}

	private:
		static_vector<int16, MAX_WORLD_SKELETON_JOINTS> _masked_joints = {};
	};

}
