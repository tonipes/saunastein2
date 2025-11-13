// Copyright (c) 2025 Inan Evin
#include "trait_physics.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"

namespace SFG
{

	void trait_physics::on_add(world& w)
	{
	}

	void trait_physics::on_remove(world& w)
	{
	}

	void trait_physics::serialize(ostream& stream, world& w) const
	{
	}

	void trait_physics::deserialize(istream& stream, world& w)
	{
	}

	void trait_physics::create_body(world& w)
	{
		SFG_ASSERT(_body == nullptr);

		physics_world& phy_world = w.get_physics_world();

		entity_manager& em	  = w.get_entity_manager();
		const vector3	pos	  = em.get_entity_position_abs(_header.entity);
		const vector3	scale = em.get_entity_scale_abs(_header.entity);
		const quat		rot	  = em.get_entity_rotation_abs(_header.entity);

		_body = phy_world.create_body(_body_type, _shape_type, _extent_or_height_radius, _material_handle, pos, rot, scale);
	}

	void trait_physics::destroy_body(world& w)
	{
		physics_world& phy_world = w.get_physics_world();
		SFG_ASSERT(_body != nullptr);
		phy_world.destroy_body(_body);
		_body = nullptr;
	}

}