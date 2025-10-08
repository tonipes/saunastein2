// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

namespace SFG
{
	template <typename T> struct pool_handle
	{
		T generation;
		T index;

		bool operator==(const pool_handle& other) const
		{
			return generation == other.generation && index == other.index;
		}

		bool is_null() const
		{
			return generation == 0;
		}
	};

	typedef pool_handle<uint16> pool_handle16;
	typedef pool_handle<uint32> pool_handle32;

}
