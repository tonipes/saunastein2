// Copyright (c) 2025 Inan Evin
#pragma once
#include <random>

namespace SFG
{
	class random
	{
	public:
		static std::mt19937& rng();
		static void			 seed_rng(uint64_t seed);

		// Uniform real in [0, 1).
		static float random_01();

		// [min_inclusive, max_inclusive].
		static int random_int(int min_inclusive, int max_inclusive);
	};

};
