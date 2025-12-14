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
		_entities = new pool_allocator_gen<world_id, world_id, MAX_ENTITIES>();
		_metas	  = new pool_allocator_simple<entity_meta, MAX_ENTITIES>();
		_families = new pool_allocator_simple<entity_family, MAX_ENTITIES>();
		//_positions		  = new pool_allocator_simple<vector3, MAX_ENTITIES>();
		//_prev_positions	  = new pool_allocator_simple<vector3, MAX_ENTITIES>();
		//_rotations		  = new pool_allocator_simple<quat, MAX_ENTITIES>();
		//_rotations_abs	  = new pool_allocator_simple<quat, MAX_ENTITIES>();
		//_prev_rotations	  = new pool_allocator_simple<quat, MAX_ENTITIES>();
		//_scales			  = new pool_allocator_simple<vector3, MAX_ENTITIES>();
		//_prev_scales	  = new pool_allocator_simple<vector3, MAX_ENTITIES>();
		_aabbs = new pool_allocator_simple<aabb, MAX_ENTITIES>();
		//_matrices			 = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		//_abs_matrices		 = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_comp_registers		 = new pool_allocator_simple<entity_comp_register, MAX_ENTITIES>();
		_local_transforms	 = new pool_allocator_simple<entity_transform, MAX_ENTITIES>();
		_abs_transforms		 = new pool_allocator_simple<entity_transform, MAX_ENTITIES>();
		_abs_transforms_prev = new pool_allocator_simple<entity_transform, MAX_ENTITIES>();
		_flags				 = new pool_allocator_simple<bitmask<uint16>, MAX_ENTITIES>();
		_abs_matrices		 = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_prev_abs_matrices	 = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_local_matrices		 = new pool_allocator_simple<matrix4x3, MAX_ENTITIES>();
		_hierarchy_orders	 = new pool_allocator_simple<world_id, MAX_ENTITIES>();
		_abs_rots			 = new pool_allocator_simple<quat, MAX_ENTITIES>();
		_prev_abs_rots		 = new pool_allocator_simple<quat, MAX_ENTITIES>();
	}

	entity_manager::~entity_manager()
	{
		delete _entities;
		delete _metas;
		delete _families;
		// delete _positions;
		// delete _prev_positions;
		// delete _rotations;
		// delete _rotations_abs;
		// delete _prev_rotations;
		// delete _scales;
		// delete _prev_scales;
		delete _aabbs;
		// delete _matrices;
		// delete _abs_matrices;
		delete _comp_registers;
		delete _local_transforms;
		delete _abs_transforms;
		delete _abs_transforms_prev;
		delete _flags;
		delete _abs_matrices;
		delete _prev_abs_matrices;
		delete _local_matrices;
		delete _hierarchy_orders;
		delete _abs_rots;
		delete _prev_abs_rots;
	}

	void entity_manager::init()
	{
	}

	void entity_manager::calculate_abs_transform(world_id e)
	{
		bitmask<uint16>& f = _flags->get(e);

		if (!f.is_set(entity_flags::entity_flags_abs_transforms_dirty_v2))
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

		f.remove(entity_flags::entity_flags_abs_transforms_dirty_v2);
	}

	void entity_manager::calculate_abs_transforms()
	{
		render_event_stream&		  stream = _world.get_render_stream();
		render_event_entity_transform update = {};

		auto& fams	   = *_families;
		auto& locals   = *_local_transforms;
		auto& abs_mats = *_abs_matrices;
		auto& rots	   = *_abs_rots;
		/*
	uint32 id = 0;

	for (world_id index : _ordered_entities)
	{
		const world_handle parent_handle = fams.get(index).parent;

		if (parent_handle.is_null())
		{
			entity_transform& local	  = locals.get(index);
			matrix4x3&		  abs	  = abs_mats.get(index);
			quat&			  abs_rot = rots.get(index);

#ifdef FIXED_FRAMERATE_ENABLED
			_prev_abs_matrices->get(index) = abs;
			_prev_abs_rots->get(index)	   = abs_rot;
#endif

			abs		= matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot = local.rotation;

			update.position	 = local.position;
			update.rotation	 = local.rotation;
			update.abs_model = abs;

#ifndef FIXED_FRAMERATE_ENABLED
			stream.add_event({.index = index, .event_type = render_event_type::update_entity_transform}, update);
#endif
			continue;
		}

		entity_transform& local			 = locals.get(index);
		matrix4x3&		  abs			 = abs_mats.get(index);
		quat&			  abs_rot		 = rots.get(index);
		matrix4x3&		  abs_parent	 = abs_mats.get(parent_handle.index);
		quat&			  parent_abs_rot = rots.get(parent_handle.index);

#ifdef FIXED_FRAMERATE_ENABLED
		_prev_abs_matrices->get(index) = abs;
		_prev_abs_rots->get(index)	   = abs_rot;
#endif

		abs		= abs_parent * matrix4x3::transform(local.position, local.rotation, local.scale);
		abs_rot = parent_abs_rot * local.rotation;

		// entity_transform& abs_transform = absolutes.get(index);
		// matrix4x3&		  abs_mat		= abs_mats.get(index);

		update.position	 = abs.get_translation();
		update.rotation	 = abs_rot;
		update.abs_model = abs;

#ifndef FIXED_FRAMERATE_ENABLED
		stream.add_event({.index = index, .event_type = render_event_type::update_entity_transform}, update);
#endif
		id++;
	}
*/

		auto& flags = *_flags;

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_handle h = *it;
			flags.get(h.index).set(entity_flags::entity_flags_abs_transforms_dirty_v2);
		}

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_id index = (*it).index;

			const bitmask<uint16>& ff = flags.get(index);
			if (!ff.is_set(entity_flags::entity_flags_is_render_proxy) || ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			if (ff.is_set(entity_flags::entity_flags_abs_transforms_dirty_v2))
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
		//_positions->reset();
		//_prev_positions->reset();
		//_rotations->reset();
		//_rotations_abs->reset();
		//_prev_rotations->reset();
		//_scales->reset();
		//_prev_scales->reset();
		//_matrices->reset();
		//_abs_matrices->reset();
		_families->reset();
		_comp_registers->reset();
		_local_transforms->reset();
		_abs_transforms->reset();
		_abs_transforms_prev->reset();
		_flags->reset();
		_abs_matrices->reset();
		_prev_abs_matrices->reset();
		_local_matrices->reset();
		_hierarchy_orders->reset();
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
		// _positions->reset(id);
		// _prev_positions->reset(id);
		// _rotations->reset(id);
		// _rotations_abs->reset(id);
		// _prev_rotations->reset(id);
		// _scales->reset(id);
		// _prev_scales->reset(id);
		//_matrices->reset(id);
		//_abs_matrices->reset(id);
		_families->reset(id);
		_comp_registers->reset(id);
		_local_transforms->reset(id);
		_abs_transforms->reset(id);
		_abs_transforms_prev->reset(id);
		_flags->reset(id);
		_abs_matrices->reset(id);
		_prev_abs_matrices->reset(id);
		_local_matrices->reset(id);
		_hierarchy_orders->reset(id);
		_abs_rots->reset(id);
		_prev_abs_rots->reset(id);
	}

	world_handle entity_manager::create_entity(const char* name)
	{
		world_handle handle = _entities->add();
		set_entity_scale(handle, vector3::one);
		_abs_transforms->get(handle.index).scale	  = vector3::one;
		_abs_transforms_prev->get(handle.index).scale = vector3::one;
		// set_entity_prev_scale_abs(handle, vector3::one);

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

	void entity_manager::build_hierarchy()
	{
		_ordered_entities.resize(0);
		_root_entity_count = 0;

		for (auto it = _entities->handles_begin(); it != _entities->handles_end(); ++it)
		{
			const world_id id = (*it).index;

			entity_family& fam = _families->get(id);
			if (!fam.parent.is_null())
				continue;

			_ordered_entities.push_back(id);

			visit_children(*it, [&](world_handle child) { _ordered_entities.push_back(child.index); });
		}

		// std::stable_sort(_ordered_entities.begin(), _ordered_entities.end(), [this](world_id id0, world_id id1) -> bool { return _hierarchy_orders->get(id0) < _hierarchy_orders->get(id1); });
		int a = 5;
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
		entity_meta& meta = _metas->get(entity.index);
		meta.flags.set(entity_flags::entity_flags_invisible, !is_visible);
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

	void entity_manager::set_entity_position_abs(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_family& fam = _families->get(entity.index);

		if (fam.parent.is_null())
		{
			set_entity_position(entity, pos);
			return;
		}

		calculate_abs_transform(fam.parent.index);
		const matrix4x3& abs = _abs_matrices->get(fam.parent.index);

		const matrix4x3 inverse_parent = abs.inverse();
		const vector3	local_position = inverse_parent * pos;
		set_entity_position(entity, local_position);
	}

	const vector3& entity_manager::get_entity_position(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		// return _positions->get(entity.index);
		return _local_transforms->get(entity.index).position;
	}

	vector3 entity_manager::get_entity_position_abs(world_handle entity)
	{
		ZoneScoped;

		SFG_ASSERT(_entities->is_valid(entity));
		return get_entity_transform_abs(entity).get_translation();
	}

	void entity_manager::set_entity_rotation(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).rotation = rot;
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

		const quat parent_abs_inv = get_entity_rotation_abs(parent).conjugate();
		const quat local_rotation = parent_abs_inv * rot;
		set_entity_rotation(entity, local_rotation);
	}

	const quat& entity_manager::get_entity_rotation(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		// return _rotations->get(entity.index);
		return _local_transforms->get(entity.index).rotation;
	}

	const quat& entity_manager::get_entity_rotation_abs(world_handle entity)
	{
		ZoneScoped;

		// entity_family& fam			  = _families->get(entity.index);
		// world_handle   parent		  = fam.parent;
		// const quat&	   local_rotation = _rotations->get(entity.index);
		//
		// if (parent.is_null())
		// 	return local_rotation;
		//
		// bitmask<uint16>& flags	 = _metas->get(entity.index).flags;
		// quat&			 abs_rot = _rotations_abs->get(entity.index);
		//
		// if (flags.is_set(entity_flags::entity_flags_abs_rotation_dirty))
		// {
		// 	abs_rot = get_entity_rotation_abs(parent) * local_rotation;
		// 	flags.remove(entity_flags::entity_flags_abs_rotation_dirty);
		// }
		//
		// return abs_rot;
		return {};
	}

	const vector3& entity_manager::get_entity_scale(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		// return _scales->get(entity.index);
		return _local_transforms->get(entity.index).scale;
	}

	vector3 entity_manager::get_entity_scale_abs(world_handle entity)
	{
		ZoneScoped;

		SFG_ASSERT(_entities->is_valid(entity));
		return get_entity_transform_abs(entity).get_scale();
	}

	void entity_manager::set_entity_scale(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).scale = scale;
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
		SFG_ASSERT(_entities->is_valid(entity));
		// matrix4x3& transform = _matrices->get(entity.index);
		//  bitmask<uint16>& flags	   = _metas->get(entity.index).flags;
		//
		//  if (flags.is_set(entity_flags::entity_flags_local_transform_dirty))
		//  {
		//  	transform = matrix4x3::transform(_positions->get(entity.index), _rotations->get(entity.index), _scales->get(entity.index));
		//  	flags.remove(entity_flags::entity_flags_local_transform_dirty);
		//  }

		// return transform;
		return {};
	}

	const matrix4x3& entity_manager::get_entity_transform_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		// bitmask<uint16>& flags		= _metas->get(entity.index).flags;
		// matrix4x3&		 abs_matrix = _abs_matrices->get(entity.index);
		//
		// if (!flags.is_set(entity_flags::entity_flags_abs_transform_dirty))
		// 	return abs_matrix;
		//
		// entity_family& fam	  = _families->get(entity.index);
		// world_handle   parent = fam.parent;
		//
		// const matrix4x3& local_transform = get_entity_transform(entity);
		// abs_matrix						 = parent.is_null() ? local_transform : (get_entity_transform_abs(parent) * local_transform);
		// flags.remove(entity_flags::entity_flags_abs_transform_dirty);
		// return abs_matrix;

		return {};
	}

	// void entity_manager::set_entity_prev_position_abs(world_handle entity, const vector3& pos)
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	_prev_positions->get(entity.index) = pos;
	// }
	//
	// void entity_manager::set_entity_prev_rotation_abs(world_handle entity, const quat& rot)
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	_prev_rotations->get(entity.index) = rot;
	// }
	//
	// void entity_manager::set_entity_prev_scale_abs(world_handle entity, const vector3& scale)
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	_prev_scales->get(entity.index) = scale;
	// }

	// const vector3& entity_manager::get_entity_prev_position_abs(world_handle entity) const
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	return _prev_positions->get(entity.index);
	// }
	//
	// const quat& entity_manager::get_entity_prev_rotation_abs(world_handle entity) const
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	return _prev_rotations->get(entity.index);
	// }
	//
	// const vector3& entity_manager::get_entity_prev_scale_abs(world_handle entity) const
	// {
	// 	SFG_ASSERT(_entities->is_valid(entity));
	// 	return _prev_scales->get(entity.index);
	// }

	void entity_manager::calculate_interpolated_transform_abs(world_handle entity, float interpolation, vector3& out_position, quat& out_rotation, vector3& out_scale)
	{
		ZoneScoped;

		SFG_ASSERT(_entities->is_valid(entity));

		entity_transform& current_abs = _abs_transforms->get(entity.index);
		entity_transform& prev_abs	  = _abs_transforms_prev->get(entity.index);
		out_position				  = vector3::lerp(prev_abs.position, current_abs.position, interpolation);
		out_scale					  = vector3::lerp(prev_abs.scale, current_abs.scale, interpolation);
		out_rotation				  = quat::slerp(prev_abs.rotation, current_abs.rotation, interpolation);

		// entity_meta&  meta			= _metas->get(entity.index);
		// const vector3 ent_pos_abs	= get_entity_position_abs(entity);
		// const quat	  ent_rot_abs	= get_entity_rotation_abs(entity);
		// const vector3 ent_scale_abs = get_entity_scale_abs(entity);
		// const float	  blend			= math::clamp(1.0f - static_cast<float>(interpolation), 0.0f, 1.0f);
		//
		// if (!meta.flags.is_set(entity_flags::entity_flags_prev_transform_init))
		// {
		// 	out_position = ent_pos_abs;
		// 	out_rotation = ent_rot_abs;
		// 	out_scale	 = ent_scale_abs;
		// 	meta.flags.set(entity_flags::entity_flags_prev_transform_init);
		// }
		// else
		// {
		// 	const vector3& ent_prev_pos_abs	  = get_entity_prev_position_abs(entity);
		// 	const quat&	   ent_prev_rot_abs	  = get_entity_prev_rotation_abs(entity);
		// 	const vector3& ent_prev_scale_abs = get_entity_prev_scale_abs(entity);
		// 	out_position					  = vector3::lerp(ent_prev_pos_abs, ent_pos_abs, blend);
		// 	out_scale						  = vector3::lerp(ent_prev_scale_abs, ent_scale_abs, blend);
		// 	out_rotation					  = quat::slerp(ent_prev_rot_abs, ent_rot_abs, blend);
		// }
		//
		// set_entity_prev_position_abs(entity, ent_pos_abs);
		// set_entity_prev_rotation_abs(entity, ent_rot_abs);
		// set_entity_prev_scale_abs(entity, ent_scale_abs);
	}

	void entity_manager::teleport_entity(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		// auto sync_prev = [this](world_handle handle) {
		// 	entity_meta&  meta		= _metas->get(handle.index);
		// 	const vector3 abs_pos	= get_entity_position_abs(handle);
		// 	const quat	  abs_rot	= get_entity_rotation_abs(handle);
		// 	const vector3 abs_scale = get_entity_scale_abs(handle);
		//
		// 	set_entity_prev_position_abs(handle, abs_pos);
		// 	set_entity_prev_rotation_abs(handle, abs_rot);
		// 	set_entity_prev_scale_abs(handle, abs_scale);
		// 	meta.flags.remove(entity_flags::entity_flags_prev_transform_init);
		// 	meta.flags.set(entity_flags::entity_flags_render_proxy_dirty);
		// };
		//
		// sync_prev(entity);
		// visit_children(entity, sync_prev);
	}

}
