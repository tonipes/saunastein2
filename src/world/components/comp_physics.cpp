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

#include "comp_physics.hpp"
#include "reflection/type_reflection.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "math/math.hpp"
#include "reflection/reflection.hpp"
#include "resources/physical_material.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace SFG
{

	void comp_physics::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_physics>::value, 0, "component");
		m.set_title("physics");
		m.add_field<&comp_physics::_body_type, comp_physics>("body_type", reflected_field_type::rf_enum, "", 0.0f, 2.0f)->_enum_list   = {"static", "kinematic", "dynamic"};
		m.add_field<&comp_physics::_shape_type, comp_physics>("shape_type", reflected_field_type::rf_enum, "", 0.0f, 4.0f)->_enum_list = {"sphere", "box", "capsule", "cylinder", "plane", "mesh"};
		m.add_field<&comp_physics::_offset, comp_physics>("offset", reflected_field_type::rf_vector3, "");
		m.add_field<&comp_physics::_extent_or_rad_height, comp_physics>("extents", reflected_field_type::rf_vector3, "");
		m.add_field<&comp_physics::_material_handle, comp_physics>("material", reflected_field_type::rf_resource, "", type_id<physical_material>::value);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			// comp_physics* c		   = static_cast<comp_physics*>(params.object_ptr);
			// const bool	  had_body = (c->_body != nullptr);
			//
			// if (had_body)
			// 	c->destroy_body(params.w);
			//
			// c->create_body(params.w);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			// comp_physics* c = static_cast<comp_physics*>(obj);
			// if (c->_body == nullptr)
			//	c->create_body(w);
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_physics* c = static_cast<comp_physics*>(obj);
			h.push_back({.handle = c->_material_handle, .type_id = type_id<physical_material>::value});
		});
	}

	void comp_physics::on_add(world& w)
	{
	}

	void comp_physics::on_remove(world& w)
	{
		if (_body)
		{
			physics_world& phy_world = w.get_physics_world();

			if (_flags.is_set(comp_physics_flags_in_sim))
				remove_from_simulation(w);

			destroy_body(w);
		}
	}

	void comp_physics::serialize(ostream& stream, world& w) const
	{
	}

	void comp_physics::deserialize(istream& stream, world& w)
	{
	}

	JPH::Body* comp_physics::create_body(world& w)
	{
		SFG_ASSERT(_body == nullptr);

		physics_world& phy_world = w.get_physics_world();

		const vector3 extent = vector3(math::almost_equal(_extent_or_rad_height.x, 0.0f) ? 1.0f : _extent_or_rad_height.x,
									   math::almost_equal(_extent_or_rad_height.y, 0.0f) ? 1.0f : _extent_or_rad_height.y,
									   math::almost_equal(_extent_or_rad_height.z, 0.0f) ? 1.0f : _extent_or_rad_height.z);

		entity_manager& em	  = w.get_entity_manager();
		const vector3	pos	  = em.get_entity_position_abs(_header.entity) + _offset;
		const vector3	scale = em.get_entity_scale_abs(_header.entity);
		const quat		rot	  = em.get_entity_rotation_abs(_header.entity);
		_body				  = phy_world.create_body(_body_type, _shape_type, extent, _material_handle, pos, rot, scale);
		return _body;
	}

	void comp_physics::destroy_body(world& w)
	{
		physics_world& phy_world = w.get_physics_world();
		SFG_ASSERT(_body != nullptr);
		phy_world.destroy_body(_body);
		_body = nullptr;
	}

	void comp_physics::add_to_simulation(world& w)
	{
		SFG_ASSERT(_body != nullptr);
		physics_world& phy_world = w.get_physics_world();
		auto		   id		 = _body->GetID();
		phy_world.add_bodies_to_world(&id, 1);
		_flags.set(comp_physics_flags_in_sim);
	}

	void comp_physics::remove_from_simulation(world& w)
	{
		SFG_ASSERT(_body != nullptr);
		physics_world& phy_world = w.get_physics_world();
		phy_world.remove_body_from_world(*_body);
		_flags.remove(comp_physics_flags_in_sim);
	}

}
