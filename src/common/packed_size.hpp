// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"

namespace SFG
{
	class packed_size
	{
	public:
		static inline uint16 float_to_half(float f)
		{
			uint32 x;
			SFG_MEMCPY(&x, &f, sizeof(x));
			uint32 sign = (x >> 16) & 0x8000u;
			uint32 mant = x & 0x007FFFFFu;
			int32  exp	= int32((x >> 23) & 0xFF) - 127 + 15; // re-bias

			if (exp <= 0)
			{
				// subnormal or underflow to zero
				if (exp < -10)
					return (uint16)sign; // ±0
				// add implicit 1, shift to align into 10-bit mantissa
				mant |= 0x00800000u;
				uint32 t = mant >> (1 - exp + 13); // 13 = 23-10
				// round to nearest
				if ((mant >> (1 - exp + 12)) & 1u)
					t += 1;
				return (uint16)(sign | t);
			}
			else if (exp >= 31)
			{
				// overflow -> inf (or keep NaN)
				if (mant == 0)
					return (uint16)(sign | 0x7C00u); // inf
				// NaN, preserve payload top bit
				return (uint16)(sign | 0x7C00u | (mant >> 13) | ((mant >> 13) == 0));
			}
			else
			{
				// normal
				uint32 h = (uint32(exp) << 10) | (mant >> 13);
				// round to nearest-even
				if (mant & 0x00001000u)
					h += 1;
				return (uint16)(sign | h);
			}
		}

		static inline uint32 pack_half2x16(float x, float y)
		{
			const uint16 hx = float_to_half(x);
			const uint16 hy = float_to_half(y);
			return (uint32(hy) << 16) | uint32(hx);
		}

		static inline uint8 pack_unorm8(float x, float range)
		{
			float u = math::clamp(x / range, 0.0f, 1.0f);
			return (uint8)math::lround(u * 255.0f);
		}

		static inline int8_t pack_snorm8(float x, float range)
		{
			float s = math::clamp(x / range, -1.0f, 1.0f);
			// Map [-1,1] to [-128,127]; clamp to avoid -128 edge cases
			int v = (int)math::lround(s * 127.0f);
			return (int8_t)math::clamp(v, -128, 127);
		}

		static inline uint32_t pack4_snorm8(float tx, float ty, float ox, float oy, float tilingRange, float offsetRange)
		{
			uint32_t b0 = (uint8)pack_snorm8(tx, tilingRange);
			uint32_t b1 = (uint8)pack_snorm8(ty, tilingRange);
			uint32_t b2 = (uint8)pack_snorm8(ox, offsetRange);
			uint32_t b3 = (uint8)pack_snorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}

		static inline uint32_t pack4_unorm8(float tx, float ty, float ox, float oy, float tilingRange, float offsetRange)
		{
			uint32_t b0 = pack_unorm8(tx, tilingRange);
			uint32_t b1 = pack_unorm8(ty, tilingRange);
			uint32_t b2 = pack_unorm8(ox, offsetRange);
			uint32_t b3 = pack_unorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}
	};
}
