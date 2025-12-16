// Copyright (c) 2025 Inan Evin
#pragma once

#include "format.hpp"

namespace SFG
{

	class render_target_definitions
	{
	public:
		static inline format get_format_gbuffer_albedo()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_gbuffer_normal()
		{
			return format::r10g0b10a2_unorm;
		}

		static inline format get_format_gbuffer_orm()
		{
			return format::r8g8b8a8_unorm;
		}

		static inline format get_format_lighting()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_post_combine()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_depth_default()
		{
			return format::d32_sfloat;
		}

		static inline format get_format_depth_default_read()
		{
			return format::r32_sfloat;
		}

		static inline format get_format_gbuffer_emissive()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_swapchain()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_ssao_ao_out()
		{
			return format::r8_unorm;
		}

		static inline format get_format_object_id()
		{
			return format::r32_uint;
		}

		static inline format get_format_selection()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_shadows()
		{
			return format::d32_sfloat;
		}

		static inline format get_format_editor()
		{
			return format::r8g8b8a8_srgb;
		}
	};
}