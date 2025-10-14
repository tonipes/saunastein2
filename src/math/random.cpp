// Copyright (c) 2025 Inan Evin

#include "random.hpp"
#include "io/assert.hpp"
#include <cstdint>
#include <chrono>
#include <cassert>

namespace SFG
{
	std::mt19937& random::rng()
	{
		thread_local std::mt19937 engine{[] {
			std::seed_seq seq{static_cast<uint32_t>(std::random_device{}()), static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
			return std::mt19937{seq};
		}()};
		return engine;
	}
	void random::seed_rng(uint64_t seed)
	{
		std::seed_seq seq{static_cast<uint32_t>(seed), static_cast<uint32_t>(seed >> 32), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
		rng() = std::mt19937{seq};
	}
	float random::random_01()
	{
		static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		return dist(rng());
	}

	int random::random_int(int min_inclusive, int max_inclusive)
	{
		SFG_ASSERT(min_inclusive <= max_inclusive);
		std::uniform_int_distribution<int> dist(min_inclusive, max_inclusive);
		return dist(rng());
	}
}
