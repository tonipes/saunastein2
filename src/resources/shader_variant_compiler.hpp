// Copyright (c) 2025 Inan Evin
#pragma once
#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/span.hpp"
#include "data/vector.hpp"

namespace SFG
{
	struct shader_raw;
	struct shader_desc;

	class shader_variant_compiler
	{
	public:
		struct compile_params
		{
			uint8				  stage;
			span<uint8>&		  data;
			const vector<string>& defines;
			const string&		  text;
			const vector<string>& folder_paths;
			bool				  compile_layout;
			span<uint8>&		  out_layout;
			const string&		  entry;
		};

		/* actual compilation */
		static bool compile(const compile_params& p);
		static bool compile_compute(const compile_params& p);

		/* compiles default pso based on desc. */
		static bool compile_raw(shader_raw& raw, const string& shader_text, const vector<string>& folder_path, const shader_desc& desc);

		/* compiles variants suitable to use in gbuffer pass. */
		static bool compile_style_gbuffer_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants suitable to use in forward transparency pass. */
		static bool compile_style_forward_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);
	};
}