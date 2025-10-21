// Copyright (c) 2025 Inan Evin

#pragma once
#include "common_world.hpp"
#include "common_entity.hpp"
#include "common/type_id.hpp"
#include "traits/common_trait.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "memory/pool_allocator.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_allocator.hpp"
#include "math/aabb.hpp"
#include "math/matrix4x3.hpp"
#include "math/quat.hpp"
#include <gui/vekt.hpp>
#include <functional>

namespace SFG
{
	class world;

	struct trait_storage
	{
		pool_allocator32 storage;
		string_id		 type_id = 0;
	};

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

		inline pool_allocator32& get_entities()
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

		pool_allocator32							 _entities		  = {};
		pool_allocator_simple<entity_meta>			 _metas			  = {};
		pool_allocator_simple<entity_family>		 _families		  = {};
		pool_allocator_simple<vector3>				 _positions		  = {};
		pool_allocator_simple<vector3>				 _prev_positions  = {};
		pool_allocator_simple<quat>					 _rotations		  = {};
		pool_allocator_simple<quat>					 _rotations_abs	  = {};
		pool_allocator_simple<quat>					 _prev_rotations  = {};
		pool_allocator_simple<vector3>				 _scales		  = {};
		pool_allocator_simple<vector3>				 _prev_scales	  = {};
		pool_allocator_simple<aabb>					 _aabbs			  = {};
		pool_allocator_simple<matrix4x3>			 _matrices		  = {};
		pool_allocator_simple<matrix4x3>			 _abs_matrices	  = {};
		pool_allocator_simple<entity_trait_register> _trait_registers = {};
	};
}
