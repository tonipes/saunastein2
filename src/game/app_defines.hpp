// Copyright (c) 2025 Inan Evin

#pragma once

namespace SFG
{

#define FIXED_FRAMERATE_ENABLED	  0
#define FIXED_FRAMERATE_MAX_TICKS 4

	static inline constexpr double FIXED_FRAMERATE_NS_D = 16'666'667.0;
	static inline constexpr int64  FIXED_FRAMERATE_NS	= static_cast<int64>(FIXED_FRAMERATE_NS_D);
	static inline constexpr float  FIXED_FRAMERATE_S	= static_cast<float>(FIXED_FRAMERATE_NS_D / 1'000'000'000.0);
}
