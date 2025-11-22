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
	class game;

	class editor_camera
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(world& world, window* wnd);
		void uninit();
		bool on_window_event(const window_event& ev);
		void tick(float dt_seconds);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void activate();
		void deactivate();

		// -----------------------------------------------------------------------------
		// accessors/muts
		// -----------------------------------------------------------------------------

		inline void set_game(game* g)
		{
			_game = g;
		}

	private:
		void reset_runtime();
		void update_rotation();
		void apply_movement(float dt_seconds);

	private:
		window* _window = nullptr;
		world*	_world	= nullptr;
		game*	_game	= nullptr;

		world_handle _entity	   = {};
		world_handle _camera_trait = {};

		vector3 _direction_input	= vector3::zero;
		vector2 _mouse_delta		= vector2::zero;
		float	_yaw_degrees		= 0.0f;
		float	_pitch_degrees		= 0.0f;
		float	_current_move_speed = 12.0f;
		float	_base_move_speed	= 12.0f;
		float	_boost_multiplier	= 8.0f;
		float	_mouse_sensitivity	= 0.08f;
		bool	_is_looking			= false;
		uint8	_is_active			= 0;
	};
}
