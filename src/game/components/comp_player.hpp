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
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "physics/physics_ray_collector.hpp"

namespace SFG
{
	class world;
	class window;
	struct window_event;

	class comp_player
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// comp
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void set_values(world& w, const color& base_color);

		void begin_game(world& w, window& wnd);
		void tick(world& w, float dt);
		void tick_debug(world& w, float dt);

		void on_window_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header = {};

		physics_raycast_single _ray_caster = {};

		world_handle _char_controller = {};
		world_handle _camera_entity	  = {};
		world_handle _camera_comp	  = {};
		world_handle _player_stats	  = {};

		vector2 _mouse_delta = vector2::zero;
		vector2 _move_input	 = vector2::zero;

		float	_movement_speed		  = 15.0f;
		float	_rotation_speed		  = 5.0f;
		float	_camera_distance	  = 4.0f;
		float	_real_camera_distance = 4.0f;
		float	_orbit_yaw_speed	  = 0.08f;
		float	_orbit_pitch_speed	  = 0.08f;
		float	_orbit_min_pitch	  = -80.0f;
		float	_orbit_max_pitch	  = 80.0f;
		float	_yaw_degrees		  = 0.0f;
		float	_pitch_degrees		  = 20.0f;
		vector3 _camera_offset		  = vector3(0.0f, 1.5f, 0.0f);
		bool	_inited				  = false;
	};

	REFLECT_TYPE(comp_player);
}
