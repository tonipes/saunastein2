// Copyright (c) 2025 Inan Evin
#include "comp_animation_controller.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"

namespace SFG
{
	void comp_animation_controller::on_add(world& w)
	{
		animation_graph& anim_graph							= w.get_animation_graph();
		_state_machine										= anim_graph.add_state_machine();
		anim_graph.get_state_machine(_state_machine).entity = _header.entity;
	}

	void comp_animation_controller::on_remove(world& w)
	{
		animation_graph& anim_graph = w.get_animation_graph();
		anim_graph.remove_state_machine(_state_machine);
	}

	void comp_animation_controller::serialize(ostream& stream, world& w) const
	{
	}

	void comp_animation_controller::deserialize(istream& stream, world& w)
	{
	}

}