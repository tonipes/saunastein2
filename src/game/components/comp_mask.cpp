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

#include "comp_mask.hpp"
#include "reflection/reflection.hpp"
#include "world/entity_manager.hpp"
#include "world/world.hpp"
#include "world/component_manager.hpp"
#include "game/components/comp_enemy_ai_basic.hpp"
#include "game/gameplay.hpp"

namespace SFG
{
	void comp_mask::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_mask>::value, 0, "component");
		m.set_title("mask");
		m.set_category("game");
		m.add_field<&comp_mask::_speed, comp_mask>("speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_mask::_hit_distance, comp_mask>("hit_distance", reflected_field_type::rf_float, "");
		m.add_field<&comp_mask::_damage, comp_mask>("damage", reflected_field_type::rf_float, "");
	}

	void comp_mask::on_add(world& w)
	{
		_is_destroyed = false;
	}

	void comp_mask::on_remove(world& w)
	{
	}

	void comp_mask::begin(world& w)
	{
		_enemies.clear();
		entity_manager& em = w.get_entity_manager();
		em.find_entities_by_tag("enemy", _enemies);

	}

	void comp_mask::update(world& w, float dt)
	{
		if (_is_destroyed)
			return;

		if (_enemies.empty())
			begin(w);

		entity_manager& em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		const quat&	   rot = em.get_entity_rotation_abs(_header.entity);
		const vector3  forward = rot.get_forward();
		const vector3  pos = em.get_entity_position_abs(_header.entity);
		em.set_entity_position_abs(_header.entity, pos + forward * (_speed * dt));

		if (_enemies.size() == 0)
			return;

		const vector3 me = em.get_entity_position_abs(_header.entity);
		const float	  dist_sq = _hit_distance * _hit_distance;

		for (world_handle h : _enemies)
		{
			if (h.is_null() || !em.is_valid(h))
				continue;

			const vector3 enemy_pos = em.get_entity_position_abs(h);
			if ((me - enemy_pos).magnitude_sqr() <= dist_sq)
			{
				const world_handle ai_handle = em.get_entity_component<comp_enemy_ai_basic>(h);
				if (!ai_handle.is_null())
				{
					comp_enemy_ai_basic& ai = cm.get_component<comp_enemy_ai_basic>(ai_handle);
					ai.take_damage(w, _damage);
				}
				gameplay::get().spawn_fx(gameplay::spawn_type::slap_effect, enemy_pos, em.get_entity_rotation_abs(h));
				_is_destroyed = true;
				break;
			}
		}
	}
}
