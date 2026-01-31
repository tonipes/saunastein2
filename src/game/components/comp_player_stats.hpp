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

#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"

namespace SFG
{
	class world;

	class comp_player_stats
	{
	public:
		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline float get_health() const
		{
			return _health;
		}

		inline float get_hydration_score() const
		{
			return _hydration_score;
		}

		inline int get_available_dive_count() const
		{
			return _available_dive_count;
		}

		inline bool can_dive() const
		{
			return _available_dive_count > 0;
		}

		void add_health(float delta);
		void add_hydration_score(float delta);
		void add_dive_count(int delta);
		bool try_consume_dive();
		void consume_dive();

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header			   = {};
		float			 _health			   = 100.0f;
		float			 _hydration_score	   = 0.0f;
		int				 _available_dive_count = 0;
	};

	REFLECT_TYPE(comp_player_stats);
}
