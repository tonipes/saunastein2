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

#include "gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "game/components/comp_player.hpp"
#include "game/components/comp_player_stats.hpp"
#include <world/components/comp_physics.hpp>
#include "game/components/comp_enemy_ai_basic.hpp"
#include <physics/physics_contact_listener.hpp>
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"
#include "resources/entity_template.hpp"

namespace SFG
{
	void gameplay::tick_doors(float dt)
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();
		physics_world&	   ph = w.get_physics_world();

		for (int i = 0; i < _doors.size(); ++i)
		{
			if (_doors[i].door_root_handle.is_null())
				continue;

			if (!_doors[i].is_opened)
			{
				vector3 player_pos = em.get_entity_position_abs(_player_entity);
				vector3 door_pos   = em.get_entity_position_abs(_doors[i].door_root_handle);
				quat	door_rot   = em.get_entity_rotation_abs(_doors[i].door_root_handle);

				float distance = (player_pos - door_pos).magnitude();
				if (distance < _doors[i].auto_open_distance)
				{
					_doors[i].is_opened			 = true;
					vector3 door_forward		 = door_rot.get_forward();
					vector3 to_player			 = (player_pos - door_pos).normalized();
					float	dot					 = vector3::dot(door_forward, to_player);
					bool	should_open_backward = dot > 0.0f;
					if (should_open_backward)
					{
						_doors[i].direction *= -1.0f;
					}
				}
			}

			if (!_doors[i].is_opened)
				continue;

			float speed = 1.0f;
			_doors[i].t += dt * speed;
			float tt = _doors[i].t;
			if (tt > 1.0f)
				tt = 1.0f;
			if (tt < 0.0f)
				tt = 0.0f;

			em.visit_children(_doors[i].door_root_handle, [&](world_handle child) {
				float ss = _doors[i].direction;
				if (em.get_entity_scale(child).x < 0.0f)
					ss *= -1.0f;

				const quat rot = quat::from_euler(0.0f, ss * tt * _doors[i].open_angle, 0.0f);
				em.set_entity_rotation(child, rot);

				world_handle  phys_ent_handle  = em.get_child_by_index(child, 0);
				world_handle  phys_comp_handle = em.get_entity_component<comp_physics>(phys_ent_handle);
				comp_physics& phys_comp		   = cm.get_component<comp_physics>(phys_comp_handle);
				phys_comp.set_body_position_and_rotation(w, em.get_entity_position_abs(phys_ent_handle), em.get_entity_rotation_abs(phys_ent_handle));
			});
		}
	}

	void gameplay::begin_doors()
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();
		physics_world&	   ph = w.get_physics_world();

		vector<world_handle> tmp = {};

		tmp.clear();
		em.find_entities_by_tag("door_root", tmp);
		for (int i = 0; i < tmp.size(); ++i)
		{
			// SFG_TRACE("DOOR: {0}", i);
			door d = {
				.door_root_handle	= tmp[i],
				.t					= 0,
				.open_angle			= 165.0f,
				.is_opened			= false,
				.auto_open_distance = 10.0f,
				.direction			= 1.0f,
			};

			_doors.push_back(d);
		}
	}

	void gameplay::check_managed_entities_collision(world_handle e1, world_handle e2)
	{
		for (int i = 0; i < _managed_entities.size(); ++i)
		{
			managed_entity& ent = _managed_entities[i];
			// if (!ent.destroy_on_collision) continue;

			if (ent.handle == e1 || ent.handle == e2)
			{
				SFG_TRACE("Managed entity collides");
				ent.marked_for_removal = true;
			}
		}
	}

	void gameplay::begin_managed_entities()
	{
		world&			  w	 = _app.get_world();
		resource_manager& rm = w.get_resource_manager();

		_managed_entities.clear();

		string_id bullet_template = "assets/entities/bullet.stkent"_hs;
		spawn_managed_entity(bullet_template, {2.0f, -5.0f, 0.0f}, {5.0f, 0.0f, 0.0f}, 100.0f);

		// for (int i = 0; i < 100; ++i)
		// {
		// 	spawn_managed_entity(bullet_template, {2.0f * i, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f * i}, 20.0f - 0.1f * i);
		// }

		SFG_TRACE("begin_managed_entities");
	}

	void gameplay::spawn_managed_entity(string_id resource, vector3 position, vector3 velocity, float max_lifetime)
	{
		world&			   w  = _app.get_world();
		resource_manager&  rm = w.get_resource_manager();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		auto res = rm.get_resource_handle_by_hash_if_exists<entity_template>(resource);

		if (res.is_null())
		{
			SFG_ERR("can't find resource to spawn! {0}", resource);
			return;
		}

		world_handle handle = em.instantiate_template(res);

		world_handle phys_comp_handle = em.get_entity_component<comp_physics>(handle);
		if (!phys_comp_handle.is_null())
		{
			comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
			phys_comp.set_body_position(w, position);
		}
		else
		{
			em.set_entity_position_abs(handle, position);
		}

		managed_entity ent = {};
		ent.handle		   = handle;
		ent.max_lifetime   = max_lifetime;
		ent.t			   = 0.0f;
		ent.velocity	   = velocity;

		_managed_entities.push_back(ent);
	}

	void gameplay::tick_managed_entities(float dt)
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();

		for (int i = 0; i < _managed_entities.size(); ++i)
		{
			managed_entity& ent = _managed_entities[i];
			ent.t += dt;

			if (!em.is_valid(ent.handle) || ent.t >= ent.max_lifetime)
			{
				ent.marked_for_removal = true;
				continue;
			}

			world_handle phys_comp_handle = em.get_entity_component<comp_physics>(ent.handle);
			if (!phys_comp_handle.is_null())
			{
				comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
				phys_comp.set_body_velocity(w, ent.velocity);
			}

			// vector3 position = em.get_entity_position_abs(ent.handle);
			// vector3 new_position = position + ent.velocity * dt;
			// em.set_entity_position_abs(ent.handle, new_position);
		}

		for (int i = _managed_entities.size() - 1; i >= 0; --i)
		{
			managed_entity& ent = _managed_entities[i];
			if (ent.marked_for_removal)
			{
				em.destroy_entity(ent.handle);
				_managed_entities.pop_back();
			}
		}
	}
}
