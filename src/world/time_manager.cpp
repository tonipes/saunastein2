// Copyright (c) 2025 Inan Evin

#include "time_manager.hpp"

namespace SFG
{
	void time_manager::init()
	{
		_elapsed_game_time = 0.0f;
		_elapsed_real_time = 0.0f;
	}

	void time_manager::uninit()
	{
	}

}