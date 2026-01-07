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

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace SFG
{

	void comp_physics::reflect()
	{
		meta& m = reflection::get().resolve(type_id<comp_physics>::value);
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

	void comp_physics::create_body(world& w)
	{
		SFG_ASSERT(_body == nullptr);

		physics_world& phy_world = w.get_physics_world();

		entity_manager& em	  = w.get_entity_manager();
		const vector3	pos	  = em.get_entity_position_abs(_header.entity);
		const vector3	scale = em.get_entity_scale_abs(_header.entity);
		const quat		rot	  = em.get_entity_rotation_abs(_header.entity);
		_body				  = phy_world.create_body(_body_type, _shape_type, _extent_or_height_radius, _material_handle, pos, rot, scale);
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
