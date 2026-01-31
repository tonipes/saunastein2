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
#include <world/components/comp_physics.hpp>
#include "game/components/comp_enemy_ai_basic.hpp"
#include <physics/physics_contact_listener.hpp>

namespace SFG
{




#define MAX_PLAYERS 1

	void gameplay::init()
	{
		_app.get_world().get_comp_manager().register_cache<comp_player, MAX_PLAYERS>();
		_app.get_world().get_comp_manager().register_cache<comp_enemy_ai_basic, MAX_WORLD_ENEMY_AI_BASIC>();
	}

	void gameplay::uninit()
	{
	}

	void gameplay::on_world_begin(world& w)
	{
		physics_world& ph = w.get_physics_world();
		ph.add_contact_listener(*this);

		begin_player();
		begin_doors();
		begin_enemies();
	}

	void gameplay::on_world_end(world& w)
	{
		_doors.clear();
	}

	void gameplay::on_world_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player(dt);
		tick_doors(dt);
		tick_enemies(dt);
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.on_window_event(ev);
	}

	void gameplay::tick_player(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick(w, dt);
	}

	void gameplay::begin_player()
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
		{
			p.begin_game(w, _app.get_main_window());
			_player_entity = p.get_header().entity;
			_player_initialized = true;
		}
	}

	void gameplay::tick_enemies(float dt)
	{
		world&	   w		 = _app.get_world();
		auto&	   enemies	 = w.get_comp_manager().underlying_pool<comp_cache<comp_enemy_ai_basic, MAX_WORLD_ENEMY_AI_BASIC>, comp_enemy_ai_basic>();
		if (!_player_initialized) return;
		const auto playerPos = w.get_entity_manager().get_entity_position(_player_entity);
		for (comp_enemy_ai_basic& e : enemies)
			e.tick(playerPos, dt);
	}

	void gameplay::begin_enemies()
	{
		world& w	   = _app.get_world();
		auto&  enemies = w.get_comp_manager().underlying_pool<comp_cache<comp_enemy_ai_basic, MAX_WORLD_ENEMY_AI_BASIC>, comp_enemy_ai_basic>();
		for (comp_enemy_ai_basic& e : enemies)
			e.begin_play(w);
	}

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
				vector3 door_pos = em.get_entity_position_abs(_doors[i].door_root_handle);
				quat door_rot = em.get_entity_rotation_abs(_doors[i].door_root_handle);

				float distance = (player_pos - door_pos).magnitude();
				if (distance < _doors[i].auto_open_distance) {
					_doors[i].is_opened = true;
					vector3 door_forward = door_rot.get_forward();
					vector3 to_player = (player_pos - door_pos).normalized();
					float	dot					 = vector3::dot(door_forward, to_player);
					bool	should_open_backward = dot > 0.0f;
					if (should_open_backward) {
						_doors[i].direction *= -1.0f;
					}
					
					SFG_TRACE("OPEN DOOR: door_forward: [{0}, {1}], to_player: [{2}, {3}]: DOT {4}, BACKWARD: {5}", door_forward.x, door_forward.z, to_player.x, to_player.z, dot, should_open_backward);
				

					//SFG_TRACE("OPEN DOOR: PLAYER: [{0}, {1}], DOOR: [{2}, {3}]: DIST {4}, BACKWARD: {5}", player_pos.x, player_pos.z, door_pos.x, door_pos.z, distance, _doors[i].direction);
				}
			}

			if (!_doors[i].is_opened) continue;

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

				const quat rot = quat::from_euler(0.0f, ss*tt * _doors[i].open_angle, 0.0f);
				em.set_entity_rotation(child, rot);

				world_handle phys_ent_handle = em.get_child_by_index(child, 0);
				world_handle phys_comp_handle = em.get_entity_component<comp_physics>(phys_ent_handle);
				comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
				phys_comp.set_body_position_and_rotation(w,
					em.get_entity_position_abs(phys_ent_handle),
					em.get_entity_rotation_abs(phys_ent_handle)
				);
			});
		}
	}

	void gameplay::begin_doors()
	{
		world&				w  = _app.get_world();
		component_manager&	cm = w.get_comp_manager();
		entity_manager&		em = w.get_entity_manager();
		physics_world&		ph	= w.get_physics_world();

		vector<world_handle> tmp = {};

		tmp.clear();
		em.find_entities_by_tag("door_root", tmp);
		for (int i = 0; i < tmp.size(); ++i)
		{
			// SFG_TRACE("DOOR: {0}", i);
			door d = {
				.door_root_handle = tmp[i],
				.t			 = 0,
				.open_angle	 = 165.0f,
				.is_opened	 = false,
				.auto_open_distance = 10.0f,
				.direction = 1.0f,
			};

			_doors.push_back(d);
		}

		//tmp.clear();
		//em.find_entities_by_tag("trigger", tmp);
		//for (int i = 0; i < tmp.size(); ++i)
		//{
		//	world_handle handle = tmp[i];
		//	world_handle phys_comp_handle = em.get_entity_component<comp_physics>(handle);
		//	comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
		//}
	}

	void gameplay::on_contact_begin(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2) {
		SFG_TRACE("on_contact_begin: {0} {0}", e1.index, e2.index);
	}

	void gameplay::on_contact(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2) {

	}
	void gameplay::on_contact_end(world_handle e1, world_handle e2) {
		SFG_TRACE("on_contact_end: {0} {0}", e1.index, e2.index);
	}
}
