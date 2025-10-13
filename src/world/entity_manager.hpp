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

		/* ---------------- entity api ---------------- */
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

		/* ---------------- entity transforms ---------------- */
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

		/* ---------------- trait api ---------------- */

		template <typename T> void init_trait_storage(uint32 max_count)
		{
			auto idx = type_id<T>::index;
			if (_traits.size() <= idx)
				_traits.resize(idx + 1);

			trait_storage& trst = _traits[idx];
			trst.type_id		= type_id<T>::value;

			pool_allocator32& stg = trst.storage;
			stg.init<T>(max_count);
		}

		template <typename T> world_handle add_trait(world_handle entity)
		{
			pool_allocator32& storage = _traits[type_id<T>::index].storage;
			world_handle	  handle  = storage.allocate<T>();
			T&				  tr	  = storage.get<T>(handle);
			tr						  = T();
			tr._header.entity		  = entity;
			tr._header.own_handle	  = handle;
			tr.on_add(_world);
			return handle;
		}

		template <typename T> T& get_trait(world_handle handle)
		{
			pool_allocator32& storage = _traits[type_id<T>::index].storage;
			return storage.get<T>(handle);
		}
		template <typename T> const pool_allocator32& get_trait_storage() const
		{
			const pool_allocator32& storage = _traits[type_id<T>::index].storage;
			return storage;
		}

		template <typename T> void remove_trait(world_handle handle)
		{
			pool_allocator32& storage = _traits[type_id<T>::index].storage;
			T&				  tr	  = storage.get<T>(handle);
			tr.on_remove(_world);
			tr.~T();
			storage.free(handle);
		}

		inline chunk_allocator32& get_traits_aux_memory()
		{
			return _traits_aux_memory;
		}

		inline static_vector<trait_storage, trait_types::trait_type_max>& get_traits()
		{
			return _traits;
		}

		/* ---------------- rest ---------------- */

		inline world& get_world()
		{
			return _world;
		}

		inline pool_allocator32& get_entities()
		{
			return _entities;
		}

	private:
		void reset_all_entity_data();
		void reset_entity_data(world_id id);

	private:
		world& _world;

		pool_allocator32					 _entities		 = {};
		pool_allocator_simple<entity_meta>	 _metas			 = {};
		pool_allocator_simple<entity_family> _families		 = {};
		pool_allocator_simple<vector3>		 _positions		 = {};
		pool_allocator_simple<vector3>		 _prev_positions = {};
		pool_allocator_simple<quat>			 _rotations		 = {};
		pool_allocator_simple<quat>			 _rotations_abs	 = {};
		pool_allocator_simple<quat>			 _prev_rotations = {};
		pool_allocator_simple<vector3>		 _scales		 = {};
		pool_allocator_simple<vector3>		 _prev_scales	 = {};
		pool_allocator_simple<aabb>			 _aabbs			 = {};
		pool_allocator_simple<matrix4x3>	 _matrices		 = {};
		pool_allocator_simple<matrix4x3>	 _abs_matrices	 = {};

		static_vector<trait_storage, trait_types::trait_type_max> _traits;
		chunk_allocator32										  _traits_aux_memory;
	};
}
