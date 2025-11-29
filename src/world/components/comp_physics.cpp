// Copyright (c) 2025 Inan Evin
#include "comp_physics.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace SFG
{

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