// Copyright (c) 2025 Inan Evin

#pragma once
#include "common_world.hpp"
#include "common_entity.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/chunk_allocator.hpp"
#include "math/aabb.hpp"
#include "math/matrix4x3.hpp"
#include "math/quat.hpp"
#include "world/world_max_defines.hpp"

namespace SFG
{
	class world;

	class entity_manager
	{
	public:
		entity_manager() = delete;
		entity_manager(world& w);
		~entity_manager();

		void init();
		void uninit();
		void post_tick(double interpolation);

		// -----------------------------------------------------------------------------
		// entity api
		// -----------------------------------------------------------------------------
		world_handle		 create_entity(const char* name = "entity");
		void				 destroy_entity(world_handle handle);
		void				 add_child(world_handle parent, world_handle child);
		void				 remove_child(world_handle parent, world_handle child);
		void				 remove_from_parent(world_handle entity);
		world_handle		 get_child_by_index(world_handle parent, uint32 index);
		const aabb&			 get_entity_aabb(world_handle entity);
		const entity_meta&	 get_entity_meta(world_handle entity) const;
		const entity_family& get_entity_family(world_handle entity) const;
		void				 on_add_render_proxy(world_handle entity);
		void				 on_remove_render_proxy(world_handle entity);
		void				 set_entity_visible(world_handle entity, bool is_visible);
		void				 remove_all_entity_traits(world_handle entity);

		inline bool is_valid(world_handle entity) const
		{
			return _entities->is_valid(entity);
		}

		template <typename VisitFunc> void visit_children(world_handle parent, VisitFunc f)
		{
			const entity_family& fam	= get_entity_family(parent);
			world_handle		 entity = fam.first_child;

			while (!entity.is_null())
			{
				f(entity);
				visit_children(entity, f);
				entity = get_entity_family(entity).next_sibling;
			}
		}

		template <typename VisitFunc> void visit_parents(world_handle entity, VisitFunc f)
		{
			const entity_family& fam	= get_entity_family(entity);
			world_handle		 parent = fam.parent;
			while (!parent.is_null())
			{
				f(parent);
				visit_parents(parent, f);
				parent = get_entity_family(parent).parent;
			}
		}

		// -----------------------------------------------------------------------------
		// transform api
		// -----------------------------------------------------------------------------
		void			 set_entity_position(world_handle entity, const vector3& pos);
		void			 set_entity_position_abs(world_handle entity, const vector3& pos);
		const vector3&	 get_entity_position(world_handle entity) const;
		vector3			 get_entity_position_abs(world_handle entity);
		void			 set_entity_rotation(world_handle entity, const quat& rot);
		void			 set_entity_rotation_abs(world_handle entity, const quat& rot);
		const quat&		 get_entity_rotation(world_handle entity) const;
		const quat&		 get_entity_rotation_abs(world_handle entity);
		void			 set_entity_scale(world_handle entity, const vector3& scale);
		void			 set_entity_scale_abs(world_handle entity, const vector3& scale);
		const vector3&	 get_entity_scale(world_handle entity) const;
		vector3			 get_entity_scale_abs(world_handle entity);
		const matrix4x3& get_entity_transform(world_handle entity);
		const matrix4x3& get_entity_transform_abs(world_handle entity);
		void			 set_entity_prev_position_abs(world_handle entity, const vector3& pos);
		void			 set_entity_prev_rotation_abs(world_handle entity, const quat& rot);
		void			 set_entity_prev_scale_abs(world_handle entity, const vector3& scale);
		const vector3&	 get_entity_prev_position_abs(world_handle entity) const;
		const quat&		 get_entity_prev_rotation_abs(world_handle entity) const;
		const vector3&	 get_entity_prev_scale_abs(world_handle entity) const;
		void			 calculate_interpolated_transform_abs(world_handle entity, float interpolation, vector3& out_position, quat& out_rotation, vector3& out_scale);
		void			 teleport_entity(world_handle entity);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world& get_world()
		{
			return _world;
		}

		inline auto get_entities()
		{
			return _entities;
		}

	private:
		friend class trait_manager;

		void on_trait_added(world_handle entity, world_handle trait_handle, string_id trait_type);
		void on_trait_removed(world_handle entity, world_handle trait_handle, string_id trait_type);
		void reset_all_entity_data();
		void reset_entity_data(world_handle handle);

	private:
		world& _world;

		pool_allocator_gen<world_id, world_id, MAX_ENTITIES>*	   _entities		= {};
		pool_allocator_simple<entity_meta, MAX_ENTITIES>		   _metas			= {};
		pool_allocator_simple<entity_family, MAX_ENTITIES>		   _families		= {};
		pool_allocator_simple<vector3, MAX_ENTITIES>			   _positions		= {};
		pool_allocator_simple<vector3, MAX_ENTITIES>			   _prev_positions	= {};
		pool_allocator_simple<quat, MAX_ENTITIES>				   _rotations		= {};
		pool_allocator_simple<quat, MAX_ENTITIES>				   _rotations_abs	= {};
		pool_allocator_simple<quat, MAX_ENTITIES>				   _prev_rotations	= {};
		pool_allocator_simple<vector3, MAX_ENTITIES>			   _scales			= {};
		pool_allocator_simple<vector3, MAX_ENTITIES>			   _prev_scales		= {};
		pool_allocator_simple<aabb, MAX_ENTITIES>				   _aabbs			= {};
		pool_allocator_simple<matrix4x3, MAX_ENTITIES>			   _matrices		= {};
		pool_allocator_simple<matrix4x3, MAX_ENTITIES>			   _abs_matrices	= {};
		pool_allocator_simple<entity_trait_register, MAX_ENTITIES> _trait_registers = {};
	};
}
