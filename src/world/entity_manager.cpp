// Copyright (c) 2025 Inan Evin

#include "entity_manager.hpp"
#include "world.hpp"
#include "world/traits/trait_mesh_instance.hpp"
#include "world/traits/trait_light.hpp"
#include "gfx/event_stream/render_event_common.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_entity.hpp"
#include "reflection/reflection.hpp"
#include "math/math.hpp"

namespace SFG
{
	entity_manager::entity_manager(world& w) : _world(w)
	{
		_traits_aux_memory.init(MAX_WORLD_TRAITS_AUX_MEMORY);

		_entities.init<world_id>(MAX_ENTITIES);
		_metas.init(MAX_ENTITIES);
		_positions.init(MAX_ENTITIES);
		_prev_positions.init(MAX_ENTITIES);
		_rotations.init(MAX_ENTITIES);
		_rotations_abs.init(MAX_ENTITIES);
		_prev_rotations.init(MAX_ENTITIES);
		_scales.init(MAX_ENTITIES);
		_prev_scales.init(MAX_ENTITIES);
		_aabbs.init(MAX_ENTITIES);
		_matrices.init(MAX_ENTITIES);
		_abs_matrices.init(MAX_ENTITIES);
		_families.init(MAX_ENTITIES);

		const auto& metas = reflection::get().get_metas();
		for (const auto& [sid, meta] : metas)
		{
			if (meta.has_function("init_trait_storage"_hs))
			{
				meta.invoke_function<void, world&>("init_trait_storage"_hs, _world);
			}
		}
	}

	entity_manager::~entity_manager()
	{
		_traits_aux_memory.uninit();

		for (trait_storage& stg : _traits)
		{
			if (stg.storage.get_raw())
				stg.storage.uninit();
		}

		_entities.uninit();
	}

	void entity_manager::init()
	{
	}

	void entity_manager::post_tick(double interpolation)
	{
		const float i = static_cast<float>(interpolation);

		entity_meta*		 metas	= &_metas.get(0);
		render_event_stream& stream = _world.get_render_stream();

		render_event_entity_transform update = {};
		for (world_handle h : _entities)
		{
			entity_meta& m = metas[h.index];
			if (m.render_proxy_count != 0 && m.flags.is_set(entity_flags::entity_flags_render_proxy_dirty))
			{
				calculate_interpolated_transform_abs(h, i, update.position, update.rotation, update.scale);
				update.abs_model = matrix4x3::transform(update.position, update.rotation, update.scale);

				stream.add_event({.index = h.index, .event_type = render_event_type::render_event_update_entity_transform}, update);
				m.flags.remove(entity_flags::entity_flags_render_proxy_dirty);
			}
		}
	}

	void entity_manager::uninit()
	{
		for (trait_storage& stg : _traits)
			stg.storage.reset();
		_traits_aux_memory.reset();
		reset_all_entity_data();
	}

	void entity_manager::reset_all_entity_data()
	{
		_entities.reset();
		_aabbs.reset();
		_metas.reset();
		_positions.reset();
		_prev_positions.reset();
		_rotations.reset();
		_rotations_abs.reset();
		_prev_rotations.reset();
		_scales.reset();
		_prev_scales.reset();
		_matrices.reset();
		_abs_matrices.reset();
		_families.reset();
	}

	void entity_manager::reset_entity_data(world_id id)
	{
		_aabbs.reset(id);
		_metas.reset(id);
		_positions.reset(id);
		_prev_positions.reset(id);
		_rotations.reset(id);
		_rotations_abs.reset(id);
		_prev_rotations.reset(id);
		_scales.reset(id);
		_prev_scales.reset(id);
		_matrices.reset(id);
		_abs_matrices.reset(id);
		_families.reset(id);
	}

	world_handle entity_manager::create_entity(const char* name)
	{
		world_handle handle = _entities.allocate<world_id>();
		set_entity_scale(handle, vector3::one);
		set_entity_prev_scale_abs(handle, vector3::one);

		entity_meta& meta = _metas.get(handle.index);
		meta.name		  = _world.get_text_allocator().allocate(name);

		return handle;
	}

	void entity_manager::destroy_entity(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		entity_meta& meta = _metas.get(entity.index);
		_world.get_text_allocator().deallocate(meta.name);

		entity_family& fam = _families.get(entity.index);

		// If parent exists, we gotta check for first-child case.
		if (!fam.parent.is_null())
		{
			SFG_ASSERT(_entities.is_valid(fam.parent));
			entity_family& fam_parent = _families.get(fam.parent.index);

			if (fam_parent.first_child == entity)
			{
				// No more first child, and we assign the next sibling if needed.
				fam_parent.first_child = {};

				if (!fam.next_sibling.is_null())
				{
					SFG_ASSERT(_entities.is_valid(fam.next_sibling));
					fam_parent.first_child = fam.next_sibling;
				}
			}
		}

		// Assign next/prev siblings
		if (!fam.prev_sibling.is_null())
		{
			SFG_ASSERT(_entities.is_valid(fam.prev_sibling));
			entity_family& fam_prev = _families.get(fam.prev_sibling.index);
			fam_prev.next_sibling	= fam.next_sibling;
		}

		if (!fam.next_sibling.is_null())
		{
			SFG_ASSERT(_entities.is_valid(fam.next_sibling));
			entity_family& fam_next = _families.get(fam.next_sibling.index);
			fam_next.prev_sibling	= fam.prev_sibling;
		}

		// Destroy children recursively.
		world_handle target_child = fam.first_child;
		while (!target_child.is_null())
		{
			world_handle next = get_entity_family(target_child).next_sibling;
			destroy_entity(target_child);
			target_child = next;
		}

		reset_entity_data(entity.index);
		_entities.free<world_id>(entity);
	}

	const aabb& entity_manager::get_entity_aabb(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _aabbs.get(entity.index);
	}

	void entity_manager::add_child(world_handle parent, world_handle child_to_add)
	{
		SFG_ASSERT(_entities.is_valid(parent));
		SFG_ASSERT(_entities.is_valid(child_to_add));
		entity_family& fam_parent = _families.get(parent.index);
		entity_family& fam_child  = _families.get(child_to_add.index);

		// remove from current parent if exists
		if (_entities.is_valid(fam_child.parent))
			remove_child(fam_child.parent, child_to_add);

		fam_child.parent = parent;

		// If first child just add.
		if (fam_parent.first_child.is_null())
		{
			fam_parent.first_child = child_to_add;
			fam_child.prev_sibling = fam_child.next_sibling = {};
		}
		else
		{
			SFG_ASSERT(_entities.is_valid(fam_parent.first_child));
			entity_family& fam_first_child = _families.get(fam_parent.first_child.index);

			// We gotta find the last child. Start with the first one, iterate until next one is null.
			world_handle last_child = fam_parent.first_child;
			world_handle next_child = last_child;
			while (!next_child.is_null())
			{
				SFG_ASSERT(_entities.is_valid(next_child));
				entity_family& fam_current = _families.get(next_child.index);
				last_child				   = next_child;
				next_child				   = fam_current.next_sibling;
			}

			SFG_ASSERT(!last_child.is_null());
			SFG_ASSERT(_entities.is_valid(last_child));
			entity_family& fam_found_last_child = _families.get(last_child.index);
			fam_found_last_child.next_sibling	= child_to_add;
			fam_child.prev_sibling				= last_child;
		}
	}

	void entity_manager::remove_child(world_handle parent, world_handle child_to_remove)
	{
		SFG_ASSERT(_entities.is_valid(parent));
		SFG_ASSERT(_entities.is_valid(child_to_remove));

		entity_family& fam_parent = _families.get(parent.index);
		entity_family& fam_child  = _families.get(child_to_remove.index);

		if (fam_parent.first_child == child_to_remove)
		{
			// invalidate first child.
			// we assign next sibling as first if applicable.
			fam_parent.first_child = {};

			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities.is_valid(fam_child.next_sibling));

				fam_parent.first_child = fam_child.next_sibling;

				// also invalidate prev sibling as it was assigned as first.
				entity_family& fam_next_sibling = _families.get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= {};
			}
		}
		else
		{
			// If not the first, there must be a prev sibling.
			// Find it, give it our current next sibling.
			SFG_ASSERT(!fam_child.prev_sibling.is_null());
			SFG_ASSERT(_entities.is_valid(fam_child.prev_sibling));

			entity_family& fam_prev_sibling = _families.get(fam_child.prev_sibling.index);
			fam_prev_sibling.next_sibling	= fam_child.next_sibling;

			// If our current next sibling is alive, give it our current prev sibling.
			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities.is_valid(fam_child.next_sibling));
				entity_family& fam_next_sibling = _families.get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= fam_child.prev_sibling;
			}
		}

		fam_child.next_sibling = {};
		fam_child.prev_sibling = {};
		fam_child.parent	   = {};
	}

	void entity_manager::remove_from_parent(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		const entity_family& family = _families.get(entity.index);
		SFG_ASSERT(_entities.is_valid(family.parent));
		remove_child(family.parent, entity);
	}

	world_handle entity_manager::get_child_by_index(world_handle entity, uint32 index)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		const entity_family& family = _families.get(entity.index);

		if (family.first_child.is_null())
			return {};

		world_handle target = family.first_child;

		for (uint32 i = 0; i < index; i++)
		{
			entity_family& fam	= _families.get(target.index);
			world_handle   next = fam.next_sibling;
			if (next.is_null())
				break;
			target = next;
		}

		return target;
	}

	const entity_meta& entity_manager::get_entity_meta(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _metas.get(entity.index);
	}

	const entity_family& entity_manager::get_entity_family(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _families.get(entity.index);
	}

	void entity_manager::on_add_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		entity_meta& meta = _metas.get(entity.index);
		meta.render_proxy_count++;
		meta.flags.set(entity_flags::entity_flags_render_proxy_dirty);
	}

	void entity_manager::on_remove_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		entity_meta& meta = _metas.get(entity.index);
		SFG_ASSERT(meta.render_proxy_count > 0);
		meta.render_proxy_count--;
		meta.flags.set(entity_flags::entity_flags_render_proxy_dirty);
		if (meta.render_proxy_count == 0)
			meta.flags.remove(entity_flags::entity_flags_prev_transform_init);
	}

	void entity_manager::set_entity_visible(world_handle entity, bool is_visible)
	{
		entity_meta& meta = _metas.get(entity.index);
		meta.flags.set(entity_flags::entity_flags_invisible, !is_visible);
	}

	/* ----------------                   ---------------- */
	/* ---------------- entity transforms ---------------- */
	/* ----------------                   ---------------- */

	void entity_manager::set_entity_position(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_positions.get(entity.index) = pos;
		_metas.get(entity.index).flags.set(entity_flags::entity_flags_local_transform_dirty | entity_flags::entity_flags_abs_transform_dirty | entity_flags::entity_flags_render_proxy_dirty);
		visit_children(entity, [this](world_handle e) { _metas.get(e.index).flags.set(entity_flags::entity_flags_abs_transform_dirty); });
	}

	void entity_manager::set_entity_position_abs(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		entity_family& fam = _families.get(entity.index);

		if (fam.parent.is_null())
		{
			set_entity_position(entity, pos);
			return;
		}

		const matrix4x3 inverse_parent = get_entity_transform_abs(fam.parent).inverse();
		const vector3	local_position = inverse_parent * pos;
		set_entity_position(entity, local_position);
	}

	const vector3& entity_manager::get_entity_position(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _positions.get(entity.index);
	}

	vector3 entity_manager::get_entity_position_abs(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return get_entity_transform_abs(entity).get_translation();
	}

	void entity_manager::set_entity_rotation(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_rotations.get(entity.index) = rot;
		_metas.get(entity.index).flags.set(entity_flags::entity_flags_local_transform_dirty | entity_flags::entity_flags_abs_transform_dirty | entity_flags::entity_flags_abs_rotation_dirty | entity_flags::entity_flags_render_proxy_dirty);
		visit_children(entity, [this](world_handle e) { _metas.get(e.index).flags.set(entity_flags::entity_flags_abs_transform_dirty | entity_flags::entity_flags_abs_rotation_dirty); });
	}

	void entity_manager::set_entity_rotation_abs(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		entity_family& fam	  = _families.get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_rotation(entity, rot);
			return;
		}

		const quat parent_abs_inv = get_entity_rotation_abs(parent).conjugate();
		const quat local_rotation = parent_abs_inv * rot;
		set_entity_rotation(entity, local_rotation);
	}

	const quat& entity_manager::get_entity_rotation(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _rotations.get(entity.index);
	}

	const quat& entity_manager::get_entity_rotation_abs(world_handle entity)
	{
		entity_family& fam			  = _families.get(entity.index);
		world_handle   parent		  = fam.parent;
		const quat&	   local_rotation = _rotations.get(entity.index);

		if (parent.is_null())
			return local_rotation;

		bitmask<uint16>& flags	 = _metas.get(entity.index).flags;
		quat&			 abs_rot = _rotations_abs.get(entity.index);

		if (flags.is_set(entity_flags::entity_flags_abs_rotation_dirty))
		{
			abs_rot = get_entity_rotation_abs(parent) * local_rotation;
			flags.remove(entity_flags::entity_flags_abs_rotation_dirty);
		}

		return abs_rot;
	}

	const vector3& entity_manager::get_entity_scale(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _scales.get(entity.index);
	}

	vector3 entity_manager::get_entity_scale_abs(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return get_entity_transform_abs(entity).get_scale();
	}

	void entity_manager::set_entity_scale(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_scales.get(entity.index) = scale;
		_metas.get(entity.index).flags.set(entity_flags::entity_flags_local_transform_dirty | entity_flags::entity_flags_abs_transform_dirty | entity_flags::entity_flags_render_proxy_dirty);
		visit_children(entity, [this](world_handle e) { _metas.get(e.index).flags.set(entity_flags::entity_flags_abs_transform_dirty); });
	}

	void entity_manager::set_entity_scale_abs(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		entity_family& fam	  = _families.get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_scale(entity, scale);
			return;
		}

		const matrix4x3 parent_abs_inv		 = get_entity_transform_abs(parent).inverse();
		const quat&		current_abs_rot		 = get_entity_rotation_abs(entity);
		const matrix4x3 rot_matrix			 = matrix4x3::rotation(current_abs_rot);
		const matrix4x3 desired_scale_matrix = matrix4x3::scale(scale);
		const matrix4x3 desired_abs_linear	 = rot_matrix * desired_scale_matrix;
		const matrix4x3 new_local_linear	 = parent_abs_inv * desired_abs_linear;
		const vector3	new_local_scale		 = new_local_linear.get_scale();
		set_entity_scale(entity, new_local_scale);
	}

	const matrix4x3& entity_manager::get_entity_transform(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		matrix4x3&		 transform = _matrices.get(entity.index);
		bitmask<uint16>& flags	   = _metas.get(entity.index).flags;

		if (flags.is_set(entity_flags::entity_flags_local_transform_dirty))
		{
			transform = matrix4x3::transform(_positions.get(entity.index), _rotations.get(entity.index), _scales.get(entity.index));
			flags.remove(entity_flags::entity_flags_local_transform_dirty);
		}

		return transform;
	}

	const matrix4x3& entity_manager::get_entity_transform_abs(world_handle entity)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		bitmask<uint16>& flags		= _metas.get(entity.index).flags;
		matrix4x3&		 abs_matrix = _abs_matrices.get(entity.index);

		if (!flags.is_set(entity_flags::entity_flags_abs_transform_dirty))
			return abs_matrix;

		entity_family& fam	  = _families.get(entity.index);
		world_handle   parent = fam.parent;

		const matrix4x3& local_transform = get_entity_transform(entity);
		abs_matrix						 = parent.is_null() ? local_transform : (get_entity_transform_abs(parent) * local_transform);
		flags.remove(entity_flags::entity_flags_abs_transform_dirty);
		return abs_matrix;
	}

	void entity_manager::set_entity_prev_position_abs(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_prev_positions.get(entity.index) = pos;
	}

	void entity_manager::set_entity_prev_rotation_abs(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_prev_rotations.get(entity.index) = rot;
	}

	void entity_manager::set_entity_prev_scale_abs(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities.is_valid(entity));
		_prev_scales.get(entity.index) = scale;
	}

	const vector3& entity_manager::get_entity_prev_position_abs(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _prev_positions.get(entity.index);
	}

	const quat& entity_manager::get_entity_prev_rotation_abs(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _prev_rotations.get(entity.index);
	}

	const vector3& entity_manager::get_entity_prev_scale_abs(world_handle entity) const
	{
		SFG_ASSERT(_entities.is_valid(entity));
		return _prev_scales.get(entity.index);
	}

	void entity_manager::calculate_interpolated_transform_abs(world_handle entity, float interpolation, vector3& out_position, quat& out_rotation, vector3& out_scale)
	{
		SFG_ASSERT(_entities.is_valid(entity));

		entity_meta&  meta			= _metas.get(entity.index);
		const vector3 ent_pos_abs	= get_entity_position_abs(entity);
		const quat	  ent_rot_abs	= get_entity_rotation_abs(entity);
		const vector3 ent_scale_abs = get_entity_scale_abs(entity);
		const float	  blend			= math::clamp(1.0f - static_cast<float>(interpolation), 0.0f, 1.0f);

		if (!meta.flags.is_set(entity_flags::entity_flags_prev_transform_init))
		{
			out_position = ent_pos_abs;
			out_rotation = ent_rot_abs;
			out_scale	 = ent_scale_abs;
			meta.flags.set(entity_flags::entity_flags_prev_transform_init);
		}
		else
		{
			const vector3& ent_prev_pos_abs	  = get_entity_prev_position_abs(entity);
			const quat&	   ent_prev_rot_abs	  = get_entity_prev_rotation_abs(entity);
			const vector3& ent_prev_scale_abs = get_entity_prev_scale_abs(entity);
			out_position					  = vector3::lerp(ent_prev_pos_abs, ent_pos_abs, blend);
			out_scale						  = vector3::lerp(ent_prev_scale_abs, ent_scale_abs, blend);
			out_rotation					  = quat::slerp(ent_prev_rot_abs, ent_rot_abs, blend);
		}

		set_entity_prev_position_abs(entity, ent_pos_abs);
		set_entity_prev_rotation_abs(entity, ent_rot_abs);
		set_entity_prev_scale_abs(entity, ent_scale_abs);
	}

}
