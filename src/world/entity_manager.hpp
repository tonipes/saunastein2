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
#include "game/game_max_defines.hpp"
#include "common/type_id.hpp"

namespace SFG
{
	class world;

	struct entity_transform
	{
		vector3 position = vector3::zero;
		vector3 scale	 = vector3::zero;
		quat	rotation = quat::identity;
	};

	struct entity_version_cache
	{
		uint32 local_version			 = 0;
		uint32 abs_version				 = 0;
		uint32 cached_parent_abs_version = 0;
		uint32 cached_local_version		 = 0;
	};

	class entity_manager
	{
	public:
		entity_manager() = delete;
		entity_manager(world& w);
		~entity_manager();

		void init();
		void uninit();
		void calculate_abs_transforms();
		void interpolate_entities(double interpolation);

		// -----------------------------------------------------------------------------
		// entity api
		// -----------------------------------------------------------------------------

		world_handle		  create_entity(const char* name = "entity");
		world_handle		  find_entity(const char* name);
		world_handle		  find_entity(world_handle parent, const char* name);
		void				  destroy_entity(world_handle handle);
		void				  add_child(world_handle parent, world_handle child);
		void				  remove_child(world_handle parent, world_handle child);
		void				  remove_from_parent(world_handle entity);
		world_handle		  get_child_by_index(world_handle parent, uint32 index);
		const aabb&			  get_entity_aabb(world_handle entity);
		const entity_meta&	  get_entity_meta(world_handle entity) const;
		const entity_family&  get_entity_family(world_handle entity) const;
		const bitmask<uint16> get_entity_flags(world_handle entity) const;
		void				  add_render_proxy(world_handle entity);
		void				  remove_render_proxy(world_handle entity);
		void				  set_entity_visible(world_handle entity, bool is_visible);
		void				  remove_all_entity_components(world_handle entity);
		world_handle		  get_valid_handle_by_index(world_id id);
		world_handle		  get_entity_component(string_id comp_type, world_handle entity);

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

		template <typename T> world_handle get_entity_trait(world_handle entity)
		{
			SFG_ASSERT(is_valid(entity));
			const entity_comp_register& reg = _comp_registers->get(entity.index);
			for (const entity_comp& t : reg.comps)
			{
				if (t.comp_type == type_id<T>::value)
					return t.comp_handle;
			}
			return {};
		}

		template <typename T> world_handle get_entity_component(world_handle entity)
		{
			const string_id id = type_id<T>::value;
			return get_entity_component(id, entity);
		}

		// -----------------------------------------------------------------------------
		// transform api
		// -----------------------------------------------------------------------------

		void			 set_entity_position(world_handle entity, const vector3& pos);
		void			 set_entity_position_abs(world_handle entity, const vector3& pos);
		void			 set_entity_rotation(world_handle entity, const quat& rot);
		void			 set_entity_rotation_abs(world_handle entity, const quat& rot);
		void			 set_entity_scale(world_handle entity, const vector3& scale);
		void			 set_entity_scale_abs(world_handle entity, const vector3& scale);
		const vector3&	 get_entity_position(world_handle entity) const;
		const quat&		 get_entity_rotation(world_handle entity) const;
		const vector3&	 get_entity_scale(world_handle entity) const;
		vector3			 get_entity_position_abs(world_handle entity);
		const quat&		 get_entity_rotation_abs(world_handle entity);
		vector3			 get_entity_scale_abs(world_handle entity);
		matrix4x3		 get_entity_matrix(world_handle entity) const;
		const matrix4x3& get_entity_matrix_abs(world_handle entity);

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

		inline void set_main_camera(world_handle entity, world_handle comp)
		{
			_camera_entity = entity;
			_camera_comp   = comp;
		}

		inline world_handle get_main_camera_entity() const
		{
			return _camera_entity;
		}

		inline world_handle get_main_camera_comp() const
		{
			return _camera_comp;
		}

	private:
		friend class component_manager;

		void calculate_abs_transform(world_id entity);
		void calculate_abs_transform_direct(world_id entity);
		void calculate_abs_rot_direct(world_id entity);
		void calculate_abs_transform_and_rot_direct(world_id entity);

		void on_component_added(world_handle entity, world_handle comp_handle, string_id comp_type);
		void on_component_removed(world_handle entity, world_handle comp_handle, string_id comp_type);
		void reset_all_entity_data();
		void reset_entity_data(world_handle handle);

	private:
		world& _world;

		pool_allocator_gen<world_id, world_id, MAX_ENTITIES>*	   _entities		  = {};
		pool_allocator_simple<entity_meta, MAX_ENTITIES>*		   _metas			  = {};
		pool_allocator_simple<entity_family, MAX_ENTITIES>*		   _families		  = {};
		pool_allocator_simple<aabb, MAX_ENTITIES>*				   _aabbs			  = {};
		pool_allocator_simple<entity_comp_register, MAX_ENTITIES>* _comp_registers	  = {};
		pool_allocator_simple<entity_transform, MAX_ENTITIES>*	   _local_transforms  = {};
		pool_allocator_simple<bitmask<uint16>, MAX_ENTITIES>*	   _flags			  = {};
		pool_allocator_simple<matrix4x3, MAX_ENTITIES>*			   _abs_matrices	  = {};
		pool_allocator_simple<matrix4x3, MAX_ENTITIES>*			   _prev_abs_matrices = {};
		pool_allocator_simple<quat, MAX_ENTITIES>*				   _abs_rots		  = {};
		pool_allocator_simple<quat, MAX_ENTITIES>*				   _prev_abs_rots	  = {};

		static_vector<world_handle, MAX_ENTITIES>* _proxy_entities = {};

		world_handle _camera_entity = {};
		world_handle _camera_comp	= {};
	};

}
