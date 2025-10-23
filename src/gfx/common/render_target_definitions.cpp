// Copyright (c) 2025 Inan Evin
#include "render_target_definitions.hpp"

namespace SFG
{

	format render_target_definitions::get_format_gbuffer_albedo()
	{
		return format::r8g8b8a8_srgb;
	}

	format render_target_definitions::get_format_gbuffer_normal()
	{
		return format::r10g0b10a2_unorm;
	}

	format render_target_definitions::get_format_gbuffer_orm()
	{
		return format::r8g8b8a8_unorm;
	}

	format render_target_definitions::get_format_lighting()
	{
		return format::r16g16b16a16_sfloat;
	}

	format render_target_definitions::get_format_depth_default()
	{
		return format::d32_sfloat;
	}

	format render_target_definitions::get_format_depth_default_read()
	{
		return format::r32_sfloat;
	}

	format render_target_definitions::get_format_gbuffer_emissive()
	{
		return format::r8g8b8a8_srgb;
	}

	format render_target_definitions::get_format_swapchain()
	{
		return format::r8g8b8a8_srgb;
	}
}
