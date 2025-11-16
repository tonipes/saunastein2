// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/bitmask.hpp"
#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"
#include "physics/common_physics.hpp"
#include "resources/common_resources.hpp"
#include "math/vector3.hpp"

namespace JPH
{
	class Body;
}

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class trait_physics
	{
	public:
		enum flags
		{
			trait_physics_flags_in_simulation = 1 << 0,
		};

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void create_body(world& w);
		void destroy_body(world& w);

		void add_to_simulation(world& w);
		void remove_from_simulation(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const trait_header& get_header() const
		{
			return _header;
		}

		inline physics_body_type get_body_type() const
		{
			return _body_type;
		}

		inline physics_shape_type get_shape_type() const
		{
			return _shape_type;
		}

		inline const vector3& get_extent_or_height_radius() const
		{
			return _extent_or_height_radius;
		}

		inline resource_handle get_material_handle() const
		{
			return _material_handle;
		}

		inline JPH::Body* get_body() const
		{
			return _body;
		}

		inline void set_body_type(physics_body_type type)
		{
			_body_type = type;
		}

		inline void set_shape_type(physics_shape_type type)
		{
			_shape_type = type;
		}

		inline void set_extent(const vector3& ext)
		{
			_extent_or_height_radius = ext;
		}

		inline void set_height_radius(float height, float radius)
		{
			_extent_or_height_radius.x = height;
			_extent_or_height_radius.y = radius;
		}

		inline void set_material_handle(resource_handle h)
		{
			_material_handle = h;
		}

		inline bitmask<uint8>& get_flags()
		{
			return _flags;
		}

	private:
		template <typename T, int> friend class trait_cache;

	private:
		trait_header	   _header					= {};
		JPH::Body*		   _body					= nullptr;
		vector3			   _extent_or_height_radius = vector3::zero;
		resource_handle	   _material_handle			= {};
		physics_body_type  _body_type				= physics_body_type::static_body;
		physics_shape_type _shape_type				= physics_shape_type::box;
		bitmask<uint8>	   _flags					= 0;
	};

	REGISTER_TRAIT(trait_physics);
}
