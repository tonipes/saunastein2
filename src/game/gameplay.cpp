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
			if (_doors[i].door_handle.is_null())
				continue;

			float speed = 1.0f;
			_doors[i].t += dt * speed;
			float tt = _doors[i].t;
			if (tt > 1.0f)
				tt = 1.0f;
			if (tt < 0.0f)
				tt = 0.0f;

			const quat rot = quat::from_euler(0.0f, tt * _doors[i].open_angle, 0.0f);
			em.set_entity_rotation(_doors[i].door_handle, rot);

			world_handle phys_ent_handle = em.get_child_by_index(_doors[i].door_handle, 0);
			world_handle phys_comp_handle = em.get_entity_component<comp_physics>(phys_ent_handle);
			comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
			phys_comp.set_body_position_and_rotation(w,
			 	em.get_entity_position_abs(phys_ent_handle),
				em.get_entity_rotation_abs(phys_ent_handle)
			);
		}
	}

	void gameplay::begin_doors()
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();

		vector<world_handle> door_handles = {};

		//em.find_entities_by_tag("door_root", door_handles);
		//for (int i = 0; i < door_handles.size(); ++i)
		//{
		//	door d = {
		//		.door_root_handle = door_handles[i],
		//		.is_opened	 = false,
		//		.t			 = 0,
		//		.open_angle	 = 165.0f,
		//	};

		//	_doors.push_back(d);
		//}

		em.find_entities_by_tag("door", door_handles);
		for (int i = 0; i < door_handles.size(); ++i)
		{
			door d = {
				.door_handle = door_handles[i],
				.t			 = 0,
				.open_angle   = 165.0f,
				.is_opened	 = false,
			};

			_doors.push_back(d);
		}
	}
}
