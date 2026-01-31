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

#include "comp_player_stats.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void comp_player_stats::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_player_stats>::value, 0, "component");
		m.set_title("player_stats");
		m.set_category("game");
		m.add_field<&comp_player_stats::_health, comp_player_stats>("health", reflected_field_type::rf_float, "");
		m.add_field<&comp_player_stats::_hydration_score, comp_player_stats>("hydration_score", reflected_field_type::rf_float, "");
		m.add_field<&comp_player_stats::_available_slow_mo_count, comp_player_stats>("available_slow_mo_count", reflected_field_type::rf_int, "");
	}

	void comp_player_stats::on_add(world& w)
	{
	}

	void comp_player_stats::on_remove(world& w)
	{
	}

	void comp_player_stats::add_health(float delta)
	{
		_health += delta;
	}

	void comp_player_stats::add_hydration_score(float delta)
	{
		_hydration_score += delta;
		if (_hydration_score >= 100.0f)
		{
			_hydration_score = 0.0f;
			++_available_slow_mo_count;
		}
	}

	void comp_player_stats::add_slow_mo_count(int delta)
	{
		const int next = _available_slow_mo_count + delta;
		_available_slow_mo_count = next < 0 ? 0 : next;
	}

	bool comp_player_stats::try_consume_slow_mo()
	{
		if (_available_slow_mo_count <= 0)
			return false;

		--_available_slow_mo_count;
		return true;
	}

	void comp_player_stats::consume_slow_mo()
	{
		if (_available_slow_mo_count > 0)
			--_available_slow_mo_count;
	}
}
