// Copyright (c) 2025 Inan Evin

#include "animation_pose.hpp"
#include "world/world.hpp"
#include "resources/animation.hpp"
#include "world/animation/animation_mask.hpp"

namespace SFG
{
	void animation_pose::sample_from_animation(world& w, resource_handle anim_handle, float time, const animation_mask& mask)
	{
		_joint_poses.resize(0);

		const auto&		   masked_joints = mask.get_mask();
		const animation&   anim			 = w.get_resource_manager().get_resource<animation>(anim_handle);
		chunk_allocator32& aux			 = w.get_resource_manager().get_aux();

		const chunk_handle32 positions		 = anim.get_position_channels();
		const chunk_handle32 rotations		 = anim.get_rotation_channels();
		const chunk_handle32 scales			 = anim.get_scale_channels();
		const uint16		 positions_count = anim.get_position_channels_count();
		const uint16		 rotations_count = anim.get_rotation_channels_count();
		const uint16		 scales_count	 = anim.get_scale_channels_count();

		const animation_channel_v3* positions_ptr = positions_count == 0 ? nullptr : aux.get<animation_channel_v3>(positions);
		const animation_channel_q*	rotations_ptr = rotations_count == 0 ? nullptr : aux.get<animation_channel_q>(rotations);
		const animation_channel_v3* scales_ptr	  = scales_count == 0 ? nullptr : aux.get<animation_channel_v3>(scales);

		for (uint16 i = 0; i < positions_count; i++)
		{
			const animation_channel_v3& ch		   = positions_ptr[i];
			const vector3				value	   = ch.sample(time, aux);
			const int16					node_index = ch.node_index;

			const int16* it_masked = masked_joints.find_if([node_index](int16 idx) -> bool { return idx == node_index; });
			if (it_masked != masked_joints.end())
				continue;

			auto it = _joint_poses.find_if([node_index](const joint_pose& p) -> bool { return p.node_index == node_index; });

			joint_pose* p = nullptr;
			if (it == _joint_poses.end())
			{
				_joint_poses.push_back({});
				p = &_joint_poses.back();
			}
			else
				p = &(*it);

			p->pos = value;
			p->flags.set(joint_pose_flags::has_position);
			p->node_index = node_index;
		}

		for (uint16 i = 0; i < rotations_count; i++)
		{
			const animation_channel_q& ch		  = rotations_ptr[i];
			const quat				   value	  = ch.sample(time, aux);
			const int16				   node_index = ch.node_index;

			const int16* it_masked = masked_joints.find_if([node_index](int16 idx) -> bool { return idx == node_index; });
			if (it_masked != masked_joints.end())
				continue;

			auto it = _joint_poses.find_if([node_index](const joint_pose& p) -> bool { return p.node_index == node_index; });

			joint_pose* p = nullptr;
			if (it == _joint_poses.end())
			{
				_joint_poses.push_back({});
				p = &_joint_poses.back();
			}
			else
				p = &(*it);

			p->rot = value;
			p->flags.set(joint_pose_flags::has_rotation);
			p->node_index = node_index;
		}

		for (uint16 i = 0; i < scales_count; i++)
		{
			const animation_channel_v3& ch		   = scales_ptr[i];
			const vector3				value	   = ch.sample(time, aux);
			const int16					node_index = ch.node_index;

			const int16* it_masked = masked_joints.find_if([node_index](int16 idx) -> bool { return idx == node_index; });
			if (it_masked != masked_joints.end())
				continue;

			auto it = _joint_poses.find_if([node_index](const joint_pose& p) -> bool { return p.node_index == node_index; });

			joint_pose* p = nullptr;
			if (it == _joint_poses.end())
			{
				_joint_poses.push_back({});
				p = &_joint_poses.back();
			}
			else
				p = &(*it);

			p->scale = value;
			p->flags.set(joint_pose_flags::has_scale);
			p->node_index = node_index;
		}
	}

	void animation_pose::blend_from(animation_pose& other, float other_ratio)
	{
		for (joint_pose& p : _joint_poses)
		{
			const int16 node_index = p.node_index;

			auto it = std::find_if(other._joint_poses.begin(), other._joint_poses.end(), [node_index](const joint_pose& other_pose) -> bool { return node_index == other_pose.node_index; });
			if (it == other._joint_poses.end())
				continue;

			const joint_pose& other = *it;

			const bool has_pos		   = p.flags.is_set(joint_pose_flags::has_position);
			const bool has_rot		   = p.flags.is_set(joint_pose_flags::has_rotation);
			const bool has_scale	   = p.flags.is_set(joint_pose_flags::has_scale);
			const bool other_has_pos   = other.flags.is_set(joint_pose_flags::has_position);
			const bool other_has_rot   = other.flags.is_set(joint_pose_flags::has_rotation);
			const bool other_has_scale = other.flags.is_set(joint_pose_flags::has_scale);

			if (has_pos && other_has_pos)
				p.pos = vector3::lerp(p.pos, other.pos, other_ratio);
			else if (!has_pos && other_has_pos)
				p.pos = other.pos;

			if (has_rot && other_has_rot)
				p.rot = quat::slerp(p.rot, other.rot, other_ratio);
			else if (!has_rot && other_has_rot)
				p.rot = other.rot;

			if (has_scale && other_has_scale)
				p.scale = vector3::lerp(p.scale, other.scale, other_ratio);
			else if (!has_scale && other_has_scale)
				p.scale = other.scale;
		}
	}

}