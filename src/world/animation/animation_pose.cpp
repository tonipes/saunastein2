// Copyright (c) 2025 Inan Evin

#include "animation_pose.hpp"
#include "world/world.hpp"
#include "resources/animation.hpp"
#include "world/animation/animation_mask.hpp"
#include "math/math.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void animation_pose::sample_from_animation(world& w, resource_handle anim_handle, float time,  const animation_mask* mask)
	{
		ZoneScoped;

		_joint_count = 0;

		const animation&   anim = w.get_resource_manager().get_resource<animation>(anim_handle);
		chunk_allocator32& aux	= w.get_resource_manager().get_aux();

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
			const int16					node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<uint16>(math::max(static_cast<int16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.pos		 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_position);
		}

		for (uint16 i = 0; i < rotations_count; i++)
		{
			const animation_channel_q& ch		  = rotations_ptr[i];
			const int16				   node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<uint16>(math::max(static_cast<int16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.rot		 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_rotation);
		}

		for (uint16 i = 0; i < scales_count; i++)
		{
			const animation_channel_v3& ch		   = scales_ptr[i];
			const int16					node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<uint16>(math::max(static_cast<int16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.scale	 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_scale);
		}
	}

	void animation_pose::blend_from(animation_pose& other, float other_ratio)
	{
		ZoneScoped;

		for (uint16 i = 0; i < _joint_count; i++)
		{
			joint_pose&			  p		= _joint_poses[i];
			const bitmask<uint8>& flags = p.flags;
			if (flags == 0)
				continue;

			const joint_pose& other_pose = other._joint_poses[i];

			const bool has_pos		   = flags.is_set(joint_pose_flags::has_position);
			const bool has_rot		   = flags.is_set(joint_pose_flags::has_rotation);
			const bool has_scale	   = flags.is_set(joint_pose_flags::has_scale);
			const bool other_has_pos   = other_pose.flags.is_set(joint_pose_flags::has_position);
			const bool other_has_rot   = other_pose.flags.is_set(joint_pose_flags::has_rotation);
			const bool other_has_scale = other_pose.flags.is_set(joint_pose_flags::has_scale);

			if (has_pos && other_has_pos)
				p.pos = vector3::lerp(p.pos, other_pose.pos, other_ratio);
			else if (!has_pos && other_has_pos)
				p.pos = other_pose.pos;

			if (has_rot && other_has_rot)
				p.rot = quat::slerp(p.rot, other_pose.rot, other_ratio);
			else if (!has_rot && other_has_rot)
				p.rot = other_pose.rot;

			if (has_scale && other_has_scale)
				p.scale = vector3::lerp(p.scale, other_pose.scale, other_ratio);
			else if (!has_scale && other_has_scale)
				p.scale = other_pose.scale;
		}
	}

}