// Copyright (c) 2025 Inan Evin
#pragma once
#include "common/size_definitions.hpp"
#include "gfx/common/descriptions.hpp"

namespace SFG
{
	enum class input_layout_type
	{
		gui_default,
	};

	enum root_param_index : uint8
	{
		rpi_engine_cbv = 0,
		rpi_constants,
		rpi_table_render_pass,
		rpi_table_material,
		rpi_table_object,
		rpi_table_dyn_sampler,
		rpi_static_sampler_begin,
	};

	enum update_pointer_index : uint8
	{
		upi_render_pass_ubo0				= 0,
		upi_render_pass_ubo1				= 1,
		upi_render_pass_ssbo0				= 2,
		upi_render_pass_ssbo1				= 3,
		upi_render_pass_ssbo2				= 4,
		upi_render_pass_ssbo3				= 5,
		upi_render_pass_ssbo4				= 6,
		upi_render_pass_texture0			= 7,
		upi_render_pass_texture1			= 8,
		upi_render_pass_texture2			= 9,
		upi_render_pass_texture3			= 10,
		upi_render_pass_texture4			= 11,
		upi_render_pass_texture_array0		= 12,
		upi_render_pass_texture_array1		= 13,
		upi_render_pass_texture_cube_array0 = 14,

		upi_material_ubo0	  = 0,
		upi_material_ssbo0	  = 1,
		upi_material_texture0 = 2,
		upi_material_texture1 = 3,
		upi_material_texture2 = 4,
		upi_material_texture3 = 5,
		upi_material_texture4 = 6,

		upi_object_ubo0		= 0,
		upi_object_texture0 = 1,

		upi_dyn_sampler0 = 0,
		upi_dyn_sampler1 = 1,
		upi_dyn_sampler2 = 2,
		upi_dyn_sampler3 = 3,
	};

	enum constant_indices : uint8
	{
		constant_index_rp_constant0 = 0,
		constant_index_rp_constant1,
		constant_index_rp_constant2,
		constant_index_rp_constant3,
		constant_index_rp_constant4,
		constant_index_rp_constant5,
		constant_index_rp_constant6,
		constant_index_rp_constant7,
		constant_index_rp_constant8,
		constant_index_rp_constant9,
		constant_index_rp_constant10,
		constant_index_rp_constant11,
		constant_index_rp_constant12,
		constant_index_mat_constant0,
		constant_index_mat_constant1,
		constant_index_mat_constant2,
		constant_index_mat_constant3,
		constant_index_mat_constant4,
		constant_index_mat_constant5,
		constant_index_object_constant0,
		constant_index_object_constant1,
		constant_index_object_constant2,
		constant_index_object_constant3,
		constant_index_object_constant4,
		constant_index_object_constant5,
		constant_index_object_constant6,
		constant_index_object_constant7,
		constant_index_object_constant8,
		constant_index_object_constant9,
		constant_index_object_constant10,
		constant_index_object_constant11,
		constant_index_object_constant12,
		constant_index_max,
	};

	struct root_constants
	{
		uint32 rp_ubo_index		  = 0;
		uint32 material_ubo_index = 0;
		uint32 object_constant0	  = 0;
		uint32 object_constant1	  = 0;
		uint32 object_constant2	  = 0;
		uint32 object_constant3	  = 0;
		uint32 object_constant4	  = 0;
		uint32 object_constant5	  = 0;
		uint32 object_constant6	  = 0;
		uint32 object_constant7	  = 0;
		uint32 object_constant8	  = 0;
	};

	class gfx_util
	{
	public:
		static gfx_id				  create_bind_layout_global(bool is_compute);
		static sampler_desc			  get_sampler_desc_anisotropic();
		static sampler_desc			  get_sampler_desc_linear();
		static sampler_desc			  get_sampler_desc_linear_repeat();
		static sampler_desc			  get_sampler_desc_nearest();
		static sampler_desc			  get_sampler_desc_nearest_repeat();
		static sampler_desc			  get_sampler_desc_gui_default();
		static sampler_desc			  get_sampler_desc_gui_text();
		static sampler_desc			  get_sampler_desc_shadow_2d();
		static sampler_desc			  get_sampler_desc_shadow_cube();
		static color_blend_attachment get_blend_attachment_alpha_blending();
		static vector<vertex_input>	  get_input_layout(input_layout_type type);
	};
}