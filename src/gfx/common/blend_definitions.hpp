// Copyright (c) 2025 Inan Evin
#pragma once

namespace SFG
{
	struct color_blend_attachment;

	enum class blend_definition_style
	{
		none,
		alpha_blend,
	};

	class blend_definitions
	{
	public:
		static void get_blend_attachment(blend_definition_style style, color_blend_attachment& out_attachment);
	};
}