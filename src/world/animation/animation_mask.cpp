// Copyright (c) 2025 Inan Evin

#include "animation_mask.hpp"
#include "world/world.hpp"
#include "resources/animation.hpp"
#include "resources/skin.hpp"
#include "resources/common_skin.hpp"

namespace SFG
{

	void animation_mask::mask_joints(world& w, resource_handle skin_handle, string_id* name_hashes, uint16 name_hashes_count)
	{
		const resource_manager&	 rm		= w.get_resource_manager();
		const chunk_allocator32& rm_aux = rm.get_aux();
		const skin&				 sk		= w.get_resource_manager().get_resource<skin>(skin_handle);

		const skin_joint* joints_ptr = rm_aux.get<skin_joint>(sk.get_joints());

		for (uint16 i = 0; i < sk.get_joints_count(); i++)
		{
			const skin_joint& j					  = joints_ptr[i];
			_joints[j.model_node_index].is_masked = 1;
		}
	}

	void animation_mask::mask_joint(world& w, resource_handle skin_handle, string_id hash)
	{
		const resource_manager&	 rm		= w.get_resource_manager();
		const chunk_allocator32& rm_aux = rm.get_aux();
		const skin&				 sk		= w.get_resource_manager().get_resource<skin>(skin_handle);

		const skin_joint* joints_ptr = rm_aux.get<skin_joint>(sk.get_joints());

		for (uint16 i = 0; i < sk.get_joints_count(); i++)
		{
			const skin_joint& j = joints_ptr[i];

			if (j.name_hash == hash)
			{
				_joints[j.model_node_index].is_masked = 1;
				break;
			}
		}
	}

}