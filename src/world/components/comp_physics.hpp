/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "data/bitmask.hpp"
#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
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

	class comp_physics
	{
	public:
		static void reflect();

		enum flags
		{
			comp_physics_flags_in_sim = 1 << 0,
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

		inline const component_header& get_header() const
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

		inline const vector3& get_offset() const
		{
			return _offset;
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
		template <typename T, int> friend class comp_cache;

	private:
		component_header   _header					= {};
		JPH::Body*		   _body					= nullptr;
		vector3			   _offset					= vector3::zero;
		vector3			   _extent_or_height_radius = vector3(1, 1, 1);
		resource_handle	   _material_handle			= {};
		physics_body_type  _body_type				= physics_body_type::static_body;
		physics_shape_type _shape_type				= physics_shape_type::box;
		bitmask<uint8>	   _flags					= 0;
	};

	REFLECT_TYPE(comp_physics);
}
