// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/common_world.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "world/world_constants.hpp"
#include <array>

namespace SFG
{
	struct window_event;
	class world;
	class window;

	class editor_first_person_controller
	{
	public:
		void init(world& world, world_handle entity, window* wnd);
		void uninit();

		void on_window_event(const window_event& ev);
		void tick(float dt_seconds);

		inline bool is_active() const
		{
			return _world != nullptr;
		}

	private:
		void update_rotation();
		void apply_movement(float dt_seconds);

	private:
		window*		 _window			 = nullptr;
		world*		 _world				 = nullptr;
		world_handle _entity			 = {};
		vector3		 _direction_input	 = vector3::zero;
		vector2		 _mouse_delta		 = vector2::zero;
		float		 _yaw_degrees		 = 0.0f;
		float		 _pitch_degrees		 = 0.0f;
		float		 _current_move_speed = 12.0f;
		float		 _base_move_speed	 = 12.0f;
		float		 _boost_multiplier	 = 8.0f;
		float		 _mouse_sensitivity	 = 0.08f;
		bool		 _is_looking		 = false;
	};
}
