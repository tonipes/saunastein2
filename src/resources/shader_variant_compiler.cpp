// Copyright (c) 2025 Inan Evin
#include "shader_variant_compiler.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/shader_description.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/common/vertex_inputs.hpp"
#include "gfx/common/blend_definitions.hpp"
#include "resources/common_resources.hpp"
#include "shader_raw.hpp"
#
namespace SFG
{
	bool shader_variant_compiler::compile(const compile_params& params)
	{
		gfx_backend* backend = gfx_backend::get();
		return backend->compile_shader_vertex_pixel(static_cast<shader_stage>(params.stage), params.text, params.defines, params.folder_paths, params.entry.c_str(), params.data, params.compile_layout, params.out_layout);
	}

	bool shader_variant_compiler::compile_compute(const compile_params& p)
	{
		gfx_backend* backend = gfx_backend::get();
		return backend->compile_shader_compute(p.text, p.folder_paths, p.entry.c_str(), p.data, p.compile_layout, p.out_layout);
	}

	bool shader_variant_compiler::compile_raw(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths, const shader_desc& desc)
	{
		/*
			Compile one compile variant depending on shader entries.
			Add one PSO completely based on raw description from the shader file.
		*/

		raw.compile_variants.push_back({});
		compile_variant& def_compile = raw.compile_variants.back();
		span<uint8>		 dummy_layout;
		if (!desc.vertex_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			const bool res = compile({
				.stage			= static_cast<uint8>(shader_stage::vertex),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.vertex_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		if (!desc.pixel_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			const bool res = compile({
				.stage			= static_cast<uint8>(shader_stage::fragment),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.pixel_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		if (!desc.compute_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::compute});

			const bool res = compile_compute({
				.stage			= static_cast<uint8>(shader_stage::compute),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.compute_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		raw.pso_variants.push_back({});
		pso_variant& pso	= raw.pso_variants.back();
		pso.desc			= desc;
		pso.compile_variant = 0;
		pso.desc.debug_name = raw.name;
		return true;
	}

	bool shader_variant_compiler::compile_style_gbuffer_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs		   = {};
		vector<vertex_input> vertex_inputs_skinned = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv, vertex_inputs);
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv_skinned, vertex_inputs_skinned);

		const vector<shader_color_attachment> color_attachments = {
			{
				.format			  = render_target_definitions::get_format_gbuffer_albedo(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_normal(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_orm(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_emissive(),
				.blend_attachment = blend_attachment,
			},
		};

		auto add_compile_var = [&](const vector<string>& defines, bool compile_ps) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<uint8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<uint8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			if (compile_ps)
			{

				def_compile.blobs.push_back({.stage = shader_stage::fragment});
				res = compile({
					.stage			= static_cast<uint8>(def_compile.blobs.back().stage),
					.data			= def_compile.blobs.back().data,
					.defines		= defines,
					.text			= shader_text,
					.folder_paths	= folder_paths,
					.compile_layout = false,
					.out_layout		= dummy_layout,
					.entry			= "PSMain",
				});

				if (!res)
				{
					def_compile.destroy();
					return false;
				}
			}

			return true;
		};

		auto add_pso = [&](uint32 compile_variant_index, const bitmask<uint32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = variant_flags.is_set(variant_flag_skinned) ? vertex_inputs_skinned : vertex_inputs;
			pso.desc.cull		 = variant_flags.is_set(variant_flag_double_sided) ? cull_mode::none : cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			bitmask<uint8> depth_flags = depth_stencil_flags::dsf_depth_test;
			depth_flags.set(depth_stencil_flags::dsf_depth_write, variant_flags.is_set(shader_variant_flags::variant_flag_z_prepass));

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = compare_op::gequal,
				.flags			   = depth_flags,
			};
		};

		// Compile-time variants.
		if (!add_compile_var({}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING"}, true))
			return false;
		if (!add_compile_var({"USE_ALPHA_CUTOFF"}, true))
			return false;
		if (!add_compile_var({"USE_ZPREPASS"}, false))
			return false;

		if (!add_compile_var({"USE_SKINNING", "USE_ALPHA_CUTOFF"}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING", "USE_ZPREPASS"}, false))
			return false;
		if (!add_compile_var({"USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING", "USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, true))
			return false;

		// PSO-variants
		add_pso(0, 0);
		add_pso(1, variant_flag_skinned);
		add_pso(2, variant_flag_alpha_cutoff);
		add_pso(3, variant_flag_z_prepass);
		add_pso(4, variant_flag_skinned | variant_flag_alpha_cutoff);
		add_pso(5, variant_flag_skinned | variant_flag_z_prepass);
		add_pso(6, variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(7, variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass);

		// double-sided
		add_pso(0, variant_flag_double_sided);
		add_pso(1, variant_flag_double_sided | variant_flag_skinned);
		add_pso(2, variant_flag_double_sided | variant_flag_alpha_cutoff);
		add_pso(3, variant_flag_double_sided | variant_flag_z_prepass);
		add_pso(4, variant_flag_double_sided | variant_flag_skinned | variant_flag_alpha_cutoff);
		add_pso(5, variant_flag_double_sided | variant_flag_skinned | variant_flag_z_prepass);
		add_pso(6, variant_flag_double_sided | variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(7, variant_flag_double_sided | variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass);

		return true;
	}
}
