// Copyright (c) 2025 Inan Evin

#include "entity_manager.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_common.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_entity.hpp"
#include "math/math.hpp"
#include "game/app_defines.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	entity_manager::entity_manager(world& w) : _world(w)
	{
		_entities		   = new pool_allocator_gen<world_id, world_id, MAX_ENTITIES>();
		_metas			   = new pool_allocator_simple<entity_meta, MAX_ENTITIES>();
		_families		   = new pool_allocator_simple<entity_family, MAX_ENTITIES>();
		_aabbs			   = new pool_allocator_simple<aabb, MAX_ENTITIES>();
		_comp_registers	   = new pool_allocator_simple<entity_comp_register, MAX_ENTITIES>();
		_local_transforms  = new pool_allocator_simple<entity_transform, MAX_ENTITIES>();
		_flags			   = new pool_allocator_simple<bitmask<uint16>, MAX_ENTITIES>();
		_abs_matrices	   = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_prev_abs_matrices = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_abs_rots		   = new pool_allocator_simple<quat, MAX_ENTITIES>();
		_prev_abs_rots	   = new pool_allocator_simple<quat, MAX_ENTITIES>();
	}

	entity_manager::~entity_manager()
	{
		delete _entities;
		delete _metas;
		delete _families;
		delete _aabbs;
		delete _comp_registers;
		delete _local_transforms;
		delete _flags;
		delete _abs_matrices;
		delete _prev_abs_matrices;
		delete _abs_rots;
		delete _prev_abs_rots;
	}

	void entity_manager::init()
	{
	}

	void entity_manager::calculate_abs_transform(world_id e)
	{
		bitmask<uint16>& f = _flags->get(e);

		if (!f.is_set(entity_flags::entity_flags_abs_transforms_dirty))
			return;

		const world_handle parent  = _families->get(e).parent;
		quat&			   abs_rot = _abs_rots->get(e);
		entity_transform&  local   = _local_transforms->get(e);

		if (parent.is_null())
		{
			matrix4x3& abs = _abs_matrices->get(e);

#ifdef FIXED_FRAMERATE_ENABLED
			_prev_abs_matrices->get(e) = abs;
			_prev_abs_rots->get(e)	   = local.rotation;
#endif
			abs		= matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot = local.rotation;
		}
		else
		{
			calculate_abs_transform(parent.index);

			const matrix4x3& parent_abs_mat = _abs_matrices->get(parent.index);
			matrix4x3&		 abs			= _abs_matrices->get(e);

#ifdef FIXED_FRAMERATE_ENABLED
			_prev_abs_matrices->get(e) = abs;
			_prev_abs_rots->get(e)	   = _abs_rots->get(e);
#endif

			abs		= parent_abs_mat * matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot = _abs_rots->get(parent.index) * local.rotation;
		}

		f.remove(entity_flags::entity_flags_abs_transforms_dirty);
	}

	void entity_manager::calculate_abs_transform_direct(world_id e)
	{
		const world_handle parent = _families->get(e).parent;
		entity_transform&  local  = _local_transforms->get(e);

		if (parent.is_null())
		{
			matrix4x3& abs = _abs_matrices->get(e);
			abs			   = matrix4x3::transform(local.position, local.rotation, local.scale);
		}
		else
		{
			calculate_abs_transform_direct(parent.index);

			const matrix4x3& parent_abs_mat = _abs_matrices->get(parent.index);
			matrix4x3&		 abs			= _abs_matrices->get(e);
			abs								= parent_abs_mat * matrix4x3::transform(local.position, local.rotation, local.scale);
		}
	}

	void entity_manager::calculate_abs_rot_direct(world_id e)
	{
		const world_handle parent  = _families->get(e).parent;
		quat&			   abs_rot = _abs_rots->get(e);
		entity_transform&  local   = _local_transforms->get(e);

		if (parent.is_null())
		{
			abs_rot = local.rotation;
		}
		else
		{
			calculate_abs_rot_direct(parent.index);
			abs_rot = _abs_rots->get(parent.index) * local.rotation;
		}
	}

	void entity_manager::calculate_abs_transform_and_rot_direct(world_id e)
	{
		const world_handle parent  = _families->get(e).parent;
		quat&			   abs_rot = _abs_rots->get(e);
		entity_transform&  local   = _local_transforms->get(e);

		if (parent.is_null())
		{
			matrix4x3& abs = _abs_matrices->get(e);
			abs			   = matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot		   = local.rotation;
		}
		else
		{
			calculate_abs_transform_and_rot_direct(parent.index);

			const matrix4x3& parent_abs_mat = _abs_matrices->get(parent.index);
			matrix4x3&		 abs			= _abs_matrices->get(e);

			abs		= parent_abs_mat * matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot = _abs_rots->get(parent.index) * local.rotation;
		}
	}

	void entity_manager::calculate_abs_transforms()
	{
		render_event_stream& stream = _world.get_render_stream();

		auto& abs_mats = *_abs_matrices;
		auto& rots	   = *_abs_rots;
		auto& flags	   = *_flags;

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_handle h = *it;
			flags.get(h.index).set(entity_flags::entity_flags_abs_transforms_dirty);
		}

		render_event_entity_transform update = {};

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_id index = (*it).index;

			const bitmask<uint16>& ff = flags.get(index);
			if (!ff.is_set(entity_flags::entity_flags_is_render_proxy) || ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			if (ff.is_set(entity_flags::entity_flags_abs_transforms_dirty))
			{
				calculate_abs_transform(index);
			}

			matrix4x3& abs_mat = abs_mats.get(index);
			quat&	   abs_rot = rots.get(index);

			update.position	 = abs_mat.get_translation();
			update.rotation	 = abs_rot;
			update.abs_model = abs_mat;
#ifndef FIXED_FRAMERATE_ENABLED
			stream.add_event({.index = index, .event_type = render_event_type::update_entity_transform}, update);
#endif
		}
	}

	void entity_manager::interpolate_entities(double interpolation)
	{
		ZoneScoped;

		render_event_stream& stream = _world.get_render_stream();

		render_event_entity_transform update = {};
		const float					  interp = math::clamp((float)interpolation, 0.0f, 1.0f);

		auto& flags = *_flags;

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_id handle_index = (*it).index;

			const bitmask<uint16>& ff = flags.get(handle_index);
			if (!ff.is_set(entity_flags::entity_flags_is_render_proxy) || ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			const matrix4x3& prev_abs	  = _prev_abs_matrices->get(handle_index);
			const matrix4x3& abs		  = _abs_matrices->get(handle_index);
			const quat&		 abs_rot	  = _abs_rots->get(handle_index);
			const quat&		 prev_abs_rot = _prev_abs_rots->get(handle_index);

			const vector3 pos = vector3::lerp(prev_abs.get_translation(), abs.get_translation(), interp);
			const quat	  rot = quat::slerp(prev_abs_rot, abs_rot, interp);
			update.position	  = pos;
			update.rotation	  = rot;

			update.abs_model = matrix4x3::transform(pos, rot, vector3::lerp(prev_abs.get_scale(), abs.get_scale(), interp));
			stream.add_event({.index = handle_index, .event_type = render_event_type::update_entity_transform}, update);
		}
	}

	void entity_manager::uninit()
	{
		reset_all_entity_data();
	}

	void entity_manager::on_component_added(world_handle entity, world_handle comp_handle, string_id comp_type)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);

		const entity_comp target = {
			.comp_type	 = comp_type,
			.comp_handle = comp_handle,
		};

		reg.comps.push_back(target);
	}

	void entity_manager::on_component_removed(world_handle entity, world_handle comp_handle, string_id comp_type)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);

		const entity_comp target = {
			.comp_type	 = comp_type,
			.comp_handle = comp_handle,
		};
		reg.comps.remove_swap(target);
	}

	void entity_manager::reset_all_entity_data()
	{
		_entities->reset();
		_aabbs->reset();
		_metas->reset();
		_families->reset();
		_comp_registers->reset();
		_local_transforms->reset();
		_flags->reset();
		_abs_matrices->reset();
		_prev_abs_matrices->reset();
		_abs_rots->reset();
		_prev_abs_rots->reset();
	}

	void entity_manager::reset_entity_data(world_handle handle)
	{
		const world_id id	= handle.index;
		entity_meta&   meta = _metas->get(id);
		_world.get_text_allocator().deallocate(meta.name);

		entity_comp_register& reg		   = _comp_registers->get(id);
		auto				  copied_comps = reg.comps;

		component_manager& cm = _world.get_comp_manager();
		for (const entity_comp& t : copied_comps)
			cm.remove_component(t.comp_type, handle, t.comp_handle);

		_aabbs->reset(id);
		_metas->reset(id);
		_families->reset(id);
		_comp_registers->reset(id);
		_local_transforms->reset(id);
		_flags->reset(id);
		_abs_matrices->reset(id);
		_prev_abs_matrices->reset(id);
		_abs_rots->reset(id);
		_prev_abs_rots->reset(id);
	}

	world_handle entity_manager::create_entity(const char* name)
	{
		world_handle handle = _entities->add();
		set_entity_scale(handle, vector3::one);

		entity_meta& meta = _metas->get(handle.index);
		meta.name		  = _world.get_text_allocator().allocate(name);

		return handle;
	}

	world_handle entity_manager::find_entity(const char* name)
	{
		const auto& entities = *_entities;
		for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
		{
			const world_handle handle = *it;
			const char*		   m	  = _metas->get(handle.index).name;
			if (strcmp(m, name) == 0)
				return handle;
		}
		return {};
	}

	world_handle entity_manager::find_entity(world_handle parent, const char* name)
	{
		SFG_ASSERT(_entities->is_valid(parent));

		const entity_family& f = _families->get(parent.index);
		world_handle		 h = {};
		visit_children(parent, [&](world_handle c) {
			const entity_meta& m = get_entity_meta(c);
			if (strcmp(m.name, name) == 0)
			{
				h = c;
				return;
			}
		});

		return h;
	}

	void entity_manager::destroy_entity(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam = _families->get(entity.index);

		// If parent exists, we gotta check for first-child case.
		if (!fam.parent.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.parent));
			entity_family& fam_parent = _families->get(fam.parent.index);

			if (fam_parent.first_child == entity)
			{
				// No more first child, and we assign the next sibling if needed.
				fam_parent.first_child = {};

				if (!fam.next_sibling.is_null())
				{
					SFG_ASSERT(_entities->is_valid(fam.next_sibling));
					fam_parent.first_child = fam.next_sibling;
				}
			}
		}

		// Assign next/prev siblings
		if (!fam.prev_sibling.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.prev_sibling));
			entity_family& fam_prev = _families->get(fam.prev_sibling.index);
			fam_prev.next_sibling	= fam.next_sibling;
		}

		if (!fam.next_sibling.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.next_sibling));
			entity_family& fam_next = _families->get(fam.next_sibling.index);
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

		remove_all_entity_components(entity);
		reset_entity_data(entity);
		_entities->remove(entity);
	}

	const aabb& entity_manager::get_entity_aabb(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _aabbs->get(entity.index);
	}

	void entity_manager::add_child(world_handle parent, world_handle child_to_add)
	{
		SFG_ASSERT(_entities->is_valid(parent));
		SFG_ASSERT(_entities->is_valid(child_to_add));
		entity_family& fam_parent = _families->get(parent.index);
		entity_family& fam_child  = _families->get(child_to_add.index);

		// remove from current parent if exists
		if (_entities->is_valid(fam_child.parent))
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
			SFG_ASSERT(_entities->is_valid(fam_parent.first_child));
			entity_family& fam_first_child = _families->get(fam_parent.first_child.index);

			// We gotta find the last child. Start with the first one, iterate until next one is null.
			world_handle last_child = fam_parent.first_child;
			world_handle next_child = last_child;
			while (!next_child.is_null())
			{
				SFG_ASSERT(_entities->is_valid(next_child));
				entity_family& fam_current = _families->get(next_child.index);
				last_child				   = next_child;
				next_child				   = fam_current.next_sibling;
			}

			SFG_ASSERT(!last_child.is_null());
			SFG_ASSERT(_entities->is_valid(last_child));
			entity_family& fam_found_last_child = _families->get(last_child.index);
			fam_found_last_child.next_sibling	= child_to_add;
			fam_child.prev_sibling				= last_child;
		}
	}

	void entity_manager::remove_child(world_handle parent, world_handle child_to_remove)
	{
		SFG_ASSERT(_entities->is_valid(parent));
		SFG_ASSERT(_entities->is_valid(child_to_remove));

		entity_family& fam_parent = _families->get(parent.index);
		entity_family& fam_child  = _families->get(child_to_remove.index);

		if (fam_parent.first_child == child_to_remove)
		{
			// invalidate first child.
			// we assign next sibling as first if applicable.
			fam_parent.first_child = {};

			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities->is_valid(fam_child.next_sibling));

				fam_parent.first_child = fam_child.next_sibling;

				// also invalidate prev sibling as it was assigned as first.
				entity_family& fam_next_sibling = _families->get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= {};
			}
		}
		else
		{
			// If not the first, there must be a prev sibling.
			// Find it, give it our current next sibling.
			SFG_ASSERT(!fam_child.prev_sibling.is_null());
			SFG_ASSERT(_entities->is_valid(fam_child.prev_sibling));

			entity_family& fam_prev_sibling = _families->get(fam_child.prev_sibling.index);
			fam_prev_sibling.next_sibling	= fam_child.next_sibling;

			// If our current next sibling is alive, give it our current prev sibling.
			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities->is_valid(fam_child.next_sibling));
				entity_family& fam_next_sibling = _families->get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= fam_child.prev_sibling;
			}
		}

		fam_child.next_sibling = {};
		fam_child.prev_sibling = {};
		fam_child.parent	   = {};
	}

	void entity_manager::remove_from_parent(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		const entity_family& family = _families->get(entity.index);
		SFG_ASSERT(_entities->is_valid(family.parent));
		remove_child(family.parent, entity);
	}

	world_handle entity_manager::get_child_by_index(world_handle entity, uint32 index)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		const entity_family& family = _families->get(entity.index);

		if (family.first_child.is_null())
			return {};

		world_handle target = family.first_child;

		for (uint32 i = 0; i < index; i++)
		{
			entity_family& fam	= _families->get(target.index);
			world_handle   next = fam.next_sibling;
			if (next.is_null())
				break;
			target = next;
		}

		return target;
	}

	const entity_meta& entity_manager::get_entity_meta(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _metas->get(entity.index);
	}

	const entity_family& entity_manager::get_entity_family(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _families->get(entity.index);
	}

	const bitmask<uint16> entity_manager::get_entity_flags(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _flags->get(entity.index);
	}

	void entity_manager::add_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_meta& meta = _metas->get(entity.index);
		meta.render_proxy_count++;
		_flags->get(entity.index).set(entity_flags::entity_flags_is_render_proxy);
	}

	void entity_manager::remove_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_meta& meta = _metas->get(entity.index);
		SFG_ASSERT(meta.render_proxy_count > 0);
		meta.render_proxy_count--;
		if (meta.render_proxy_count == 0)
			_flags->get(entity.index).remove(entity_flags::entity_flags_is_render_proxy);
	}

	void entity_manager::set_entity_visible(world_handle entity, bool is_visible)
	{
		SFG_ASSERT(is_valid(entity));
		_flags->get(entity.index).set(entity_flags::entity_flags_invisible, !is_visible);
	}

	void entity_manager::remove_all_entity_components(world_handle entity)
	{
		SFG_ASSERT(is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);
		component_manager&	  cm  = _world.get_comp_manager();

		for (const entity_comp& c : reg.comps)
		{
			cm.remove_component(c.comp_type, entity, c.comp_handle);
		}

		reg.comps.resize(0);
	}

	world_handle entity_manager::get_valid_handle_by_index(world_id id)
	{
		const world_id gen = _entities->get_generation(id);
		return {
			.generation = gen,
			.index		= id,
		};
	}

	world_handle entity_manager::get_entity_component(string_id comp_type, world_handle entity)
	{
		entity_comp_register& reg = _comp_registers->get(entity.index);
		for (const entity_comp& c : reg.comps)
		{
			if (c.comp_type == comp_type)
				return c.comp_handle;
		}

		return {};
	}

	/* ----------------                   ---------------- */
	/* ---------------- entity transforms ---------------- */
	/* ----------------                   ---------------- */

	void entity_manager::set_entity_position(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).position = pos;
	}

	void entity_manager::set_entity_rotation(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).rotation = rot;
	}

	void entity_manager::set_entity_scale(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).scale = scale;
	}

	void entity_manager::set_entity_position_abs(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_position(entity, pos);
			return;
		}

		calculate_abs_transform_direct(parent.index);
		const matrix4x3& parent_abs		= _abs_matrices->get(parent.index);
		const vector3	 local_position = parent_abs.inverse() * pos;
		set_entity_position(entity, local_position);
	}

	void entity_manager::set_entity_rotation_abs(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_rotation(entity, rot);
			return;
		}

		calculate_abs_rot_direct(parent.index);
		const quat& parent_rot	   = _abs_rots->get(parent.index);
		const quat	local_rotation = parent_rot.conjugate() * rot;
		set_entity_rotation(entity, local_rotation);
	}

	void entity_manager::set_entity_scale_abs(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_scale(entity, scale);
			return;
		}

		calculate_abs_transform_direct(parent.index);

		const matrix4x3& parent_abs			  = _abs_matrices->get(parent.index);
		const matrix4x3	 parent_abs_inv		  = parent_abs.inverse();
		const quat&		 current_abs_rot	  = get_entity_rotation_abs(entity);
		const matrix4x3	 rot_matrix			  = matrix4x3::rotation(current_abs_rot);
		const matrix4x3	 desired_scale_matrix = matrix4x3::scale(scale);
		const matrix4x3	 desired_abs_linear	  = rot_matrix * desired_scale_matrix;
		const matrix4x3	 new_local_linear	  = parent_abs_inv * desired_abs_linear;
		const vector3	 new_local_scale	  = new_local_linear.get_scale();
		set_entity_scale(entity, new_local_scale);
	}

	const vector3& entity_manager::get_entity_position(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).position;
	}

	const quat& entity_manager::get_entity_rotation(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).rotation;
	}

	const vector3& entity_manager::get_entity_scale(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).scale;
	}

	vector3 entity_manager::get_entity_position_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		calculate_abs_transform_direct(entity.index);
		const matrix4x3& abs = _abs_matrices->get(entity.index);
		return abs.get_translation();
	}

	const quat& entity_manager::get_entity_rotation_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		calculate_abs_rot_direct(entity.index);
		return _abs_rots->get(entity.index);
	}

	vector3 entity_manager::get_entity_scale_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		calculate_abs_transform_direct(entity.index);
		const matrix4x3& abs = _abs_matrices->get(entity.index);
		return abs.get_scale();
	}

	matrix4x3 entity_manager::get_entity_matrix(world_handle entity) const
	{
		const entity_transform& local = _local_transforms->get(entity.index);
		return matrix4x3::transform(local.position, local.rotation, local.scale);
	}

	const matrix4x3& entity_manager::get_entity_matrix_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		calculate_abs_transform_direct(entity.index);
		return _abs_matrices->get(entity.index);
	}

}
