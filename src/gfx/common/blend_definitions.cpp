// Copyright (c) 2025 Inan Evin
#include "blend_definitions.hpp"
#include "gfx/common/shader_description.hpp"

namespace SFG
{
	void blend_definitions::get_blend_attachment(blend_definition_style style, color_blend_attachment& out_attachment)
	{
		if (style == blend_definition_style::none)
		{
			out_attachment = {
				.blend_enabled	  = false,
				.color_comp_flags = ccf_red | ccf_green | ccf_blue | ccf_alpha,
			};
		}
		else if (style == blend_definition_style::alpha_blend)
		{
			out_attachment = {
				.src_color_blend_factor = blend_factor::src_alpha,
				.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
				.color_blend_op			= blend_op::add,
				.src_alpha_blend_factor = blend_factor::one,
				.dst_alpha_blend_factor = blend_factor::zero,
				.alpha_blend_op			= blend_op::add,
				.color_comp_flags		= ccf_red | ccf_green | ccf_blue | ccf_alpha,
			};
		}
	}
}
