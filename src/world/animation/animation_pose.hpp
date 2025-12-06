// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "data/bitmask.hpp"
#include "data/static_vector.hpp"
#include "game/game_max_defines.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	class world;
	class animation;
	class skin;
	class animation_mask;

	enum joint_pose_flags : uint8
	{
		has_position = 1 << 0,
		has_rotation = 1 << 1,
		has_scale	 = 1 << 2,
	};

	struct joint_pose
	{
		int16		   node_index = -1;
		vector3		   pos		  = vector3::zero;
		quat		   rot		  = quat::identity;
		vector3		   scale	  = vector3::zero;
		bitmask<uint8> flags	  = 0;
	};

	class animation_pose
	{
	public:
		void sample_from_animation(world& w, resource_handle anim, float time, const animation_mask& mask);
		void blend_from(animation_pose& other, float other_ratio);

		inline void reset()
		{
			_joint_poses.resize(0);
		}

		inline const static_vector<joint_pose, MAX_WORLD_SKELETON_JOINTS>& get_joint_poses() const
		{
			return _joint_poses;
		}

	private:
		static_vector<joint_pose, MAX_WORLD_SKELETON_JOINTS> _joint_poses = {};
	};

}
