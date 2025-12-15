// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"

namespace SFG
{
	class time_manager
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		inline float tick(float dt)
		{
			const float modified = dt * _time_speed;
			_elapsed_real_time += dt;
			_elapsed_game_time += modified;
			return modified;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_time_speed(float s)
		{
			_time_speed = s;

			if (_time_speed < 0.0f)
				_time_speed = 0.0f;
		}

		inline float get_time_speed() const
		{
			return _time_speed;
		}

		inline float get_elapsed_game_time() const
		{
			return _elapsed_game_time;
		}

		inline float get_elapsed_real_time() const
		{
			return _elapsed_real_time;
		}

		inline float get_game_dt(float in_dt)
		{
			return in_dt * _time_speed;
		}

	private:
		float _elapsed_real_time = 0.0f;
		float _elapsed_game_time = 0.0f;
		float _time_speed		 = 1.0f;
	};
}
