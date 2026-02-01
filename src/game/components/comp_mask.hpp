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
#include "data/vector.hpp"

namespace SFG
{
	class world;

	class comp_mask
	{
	public:
		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);
		void begin(world& w);
		void update(world& w, float dt);

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline bool is_destroyed() const
		{
			return _is_destroyed;
		}

		inline float get_speed() const
		{
			return _speed;
		}

		inline void set_speed(float speed)
		{
			_speed = speed;
		}

		inline float get_damage() const
		{
			return _damage;
		}

		inline void set_damage(float damage)
		{
			_damage = damage;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header	   _header			  = {};
		vector<world_handle> _enemies			  = {};
		float				   _speed			  = 20.0f;
		float				   _hit_distance	  = 2.0f;
		float				   _damage			  = 10.0f;
		bool				   _is_destroyed	  = false;
	};

	REFLECT_TYPE(comp_mask);
}
