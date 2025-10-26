// Copyright (c) 2025 Inan Evin
#pragma once

#include "format.hpp"

namespace SFG
{

	class render_target_definitions
	{
	public:
		static format get_format_gbuffer_albedo();
		static format get_format_gbuffer_normal();
		static format get_format_gbuffer_emissive();
		static format get_format_gbuffer_orm();
		static format get_format_lighting();
		static format get_format_depth_default();
		static format get_format_shadows();
		static format get_format_depth_default_read();
		static format get_format_swapchain();
	};
}