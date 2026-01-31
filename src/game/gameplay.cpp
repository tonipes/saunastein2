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

namespace SFG
{

#define MAX_PLAYERS 1

	void gameplay::init()
	{
		_app.get_world().get_comp_manager().register_cache<comp_player, MAX_PLAYERS>();
		_app.get_world().get_comp_manager().register_cache<comp_player_stats, MAX_PLAYERS>();
	}

	void gameplay::uninit()
	{
	}

	void gameplay::on_world_begin(world& w)
	{
		w.get_physics_world().set_contact_listener(this);
		w.get_physics_world().set_character_contact_listener(this);
		begin_player();
		begin_doors();
	}

	void gameplay::on_world_end(world& w)
	{
		w.get_physics_world().set_contact_listener(nullptr);
		w.get_physics_world().set_character_contact_listener(nullptr);
		_doors.clear();
	}

	void gameplay::on_debug_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player_debug(dt);
	}

	void gameplay::on_world_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player(dt);
		tick_doors(dt);
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.on_window_event(ev);
	}

	void gameplay::on_contact_begin(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2)
	{
	}

	void gameplay::on_contact(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2)
	{
	}

	void gameplay::on_contact_end(world_handle e1, world_handle e2)
	{
	}

	void gameplay::on_character_contact_begin(world_handle character, world_handle other, const vector3& position, const vector3& normal)
	{
	}

	void gameplay::on_character_contact(world_handle character, world_handle other, const vector3& position, const vector3& normal)
	{
	}

	void gameplay::on_character_contact_end(world_handle character, world_handle other)
	{
		(void)character;
		(void)other;
	}

	void gameplay::tick_player(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick(w, dt);
	}

	void gameplay::tick_player_debug(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick_debug(w, dt);
	}

	void gameplay::begin_player()
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.begin_game(w, _app.get_main_window());
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
			if (_doors[i].t > 1.0f)
				_doors[i].t = 1.0f;

			const quat rot = quat::from_euler(0.0f, _doors[i].t * _doors[i].open_angle, 0.0f);
			em.set_entity_rotation(_doors[i].door_handle, rot);

			world_handle  phys_ent_handle  = em.get_child_by_index(_doors[i].door_handle, 0);
			world_handle  phys_comp_handle = em.get_entity_component<comp_physics>(phys_ent_handle);
			comp_physics& phys_comp		   = cm.get_component<comp_physics>(phys_comp_handle);
			phys_comp.set_body_position_and_rotation(w,
													 // vector3(0.0f, 0.0f, 0.0f),
													 em.get_entity_position_abs(phys_ent_handle),
													 em.get_entity_rotation_abs(phys_ent_handle));
		}
	}

	void gameplay::begin_doors()
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();

		vector<world_handle> door_handles = {};
		em.find_entities_by_tag("door", door_handles);
		// SFG_TRACE("DOOR COUNT {0}", door_handles.size());
		for (int i = 0; i < door_handles.size(); ++i)
		{
			SFG_TRACE("DOOR {0} {1}", i, door_handles[i].index);

			// SFG_TRACE("DOOR {0} {1}", i, door_handles[i].index);

			door d = {
				.door_handle = door_handles[i],
				.t			 = 0,
				.open_angle	 = 165.0f,
			};

			_doors.push_back(d);
		}
	}
}
