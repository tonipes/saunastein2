// Copyright (c) 2025 Inan Evin

#include "render_pass_pre_depth.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/renderer.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "world/world.hpp"
#include "resources/vertex.hpp"
#include "math/vector2ui16.hpp"
#include "resources/shader_raw.hpp"

namespace SFG
{
	void render_pass_pre_depth::init(const init_params& params)
	{
		_alloc.init(params.alloc, params.alloc_size);

		gfx_backend* backend = gfx_backend::get();

		create_textures(params.size);

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "depth_cmd"});
			pfd.ubo.create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "opaque_ubo"});

			pfd.bind_group = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(pfd.bind_group, rpi_table_render_pass, upi_render_pass_ssbo1 + 1, false);
			backend->bind_group_update_pointer(pfd.bind_group,
											   0,
											   {
												   {.resource = pfd.ubo.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
												   {.resource = params.entities[i], .view = 0, .pointer_index = upi_render_pass_ssbo0, .type = binding_type::ssbo},
												   {.resource = params.bones[i], .view = 0, .pointer_index = upi_render_pass_ssbo1, .type = binding_type::ssbo},
											   });
		}
	}

	void render_pass_pre_depth::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			backend->destroy_bind_group(pfd.bind_group);
			pfd.ubo.destroy();
		}

		destroy_textures();
	}

	void render_pass_pre_depth::prepare(proxy_manager& pm, const renderable_collector& collector, uint8 frame_index)
	{
		_draws.clear();

		const auto& renderables = collector.get_renderables();

		for (const renderable_object& obj : renderables)
		{
			const render_proxy_material& proxy_material = pm.get_material(obj.material);
			if (proxy_material.flags.is_set(material_flags::material_flags_is_forward))
				continue;
			const gfx_id	bg				= proxy_material.bind_groups[frame_index];
			const gfx_id	base_shader		= proxy_material.shader_handle;
			const bitmask32 mat_flags		= proxy_material.flags;
			const bool		is_alpha_cutoff = mat_flags.is_set(material_flags::material_flags_is_alpha_cutoff);
			const bool		is_double_sided = mat_flags.is_set(material_flags::material_flags_is_double_sided);

			bitmask<uint32> variant_flags = shader_variant_flags::variant_flag_z_prepass;
			variant_flags.set(shader_variant_flags::variant_flag_alpha_cutoff, is_alpha_cutoff);
			variant_flags.set(shader_variant_flags::variant_flag_skinned, obj.is_skinned);
			variant_flags.set(shader_variant_flags::variant_flag_double_sided, is_double_sided);

			const gfx_id target_shader = pm.get_shader_variant(base_shader, variant_flags.value());
			SFG_ASSERT(target_shader != NULL_GFX_ID);

			_draws.push_back({
				.constants =
					{
						.constant0 = obj.gpu_entity,
					},
				.base_vertex	= obj.vertex_start,
				.index_count	= obj.index_count,
				.instance_count = 1,
				.start_index	= obj.index_start,
				.start_instance = 0,
				.pipeline		= target_shader,
				.bind_group		= bg,
				.vertex_buffer	= obj.vertex_buffer->get_hw_gpu(),
				.idx_buffer		= obj.index_buffer->get_hw_gpu(),
			});
		}

		std::sort(_draws.begin(), _draws.end(), [](const indexed_draw& d1, const indexed_draw& d2) -> bool { return d1.pipeline < d2.pipeline; });

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj = collector.get_view().view_proj_matrix,
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_pre_depth::render(const render_params& p)
	{
		gfx_backend*	backend		  = gfx_backend::get();
		per_frame_data& pfd			  = _pfd[p.frame_index];
		const gfx_id	cmd_buffer	  = pfd.cmd_buffer;
		const gfx_id	depth_texture = pfd.depth_texture;
		const gfx_id	rp_bind_group = pfd.bind_group;

		static_vector<barrier, 1> barriers;
		static_vector<barrier, 1> barriers_after;

		barriers.push_back({
			.resource	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_depth_write,
		});

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "depth_pre_pass");

		backend->cmd_begin_render_pass_depth_only(cmd_buffer,
												  {
													  .depth_stencil_attachment =
														  {
															  .texture		  = depth_texture,
															  .clear_stencil  = 0,
															  .clear_depth	  = 0.0f,
															  .depth_load_op  = load_op::clear,
															  .depth_store_op = store_op::store,
															  .view_index	  = 0,
														  },
												  });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});
		backend->cmd_bind_group(cmd_buffer, {.group = rp_bind_group});
		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),

								  });

		gfx_id last_bound_group	   = NULL_GFX_ID;
		gfx_id last_bound_pipeline = NULL_GFX_ID;
		gfx_id last_vtx			   = NULL_GFX_ID;
		gfx_id last_idx			   = NULL_GFX_ID;

		auto bind = [&](gfx_id group, gfx_id pipeline, gfx_id vtx, gfx_id idx) {
			if (vtx != last_vtx)
			{
				last_vtx = vtx;
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = vtx, .vertex_size = sizeof(vertex_static)});
			}

			if (idx != last_idx)
			{
				last_idx = idx;
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = idx, .index_size = static_cast<uint8>(sizeof(primitive_index))});
			}
			if (pipeline != last_bound_pipeline)
			{
				last_bound_pipeline = pipeline;
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = pipeline});
			}

			if (group != last_bound_group)
			{
				last_bound_group = group;
				backend->cmd_bind_group(cmd_buffer, {.group = group});
			}
		};

		for (const indexed_draw& draw : _draws)
		{
			bind(draw.bind_group, draw.pipeline, draw.vertex_buffer, draw.idx_buffer);

			backend->cmd_bind_constants(cmd_buffer, {.data = (void*)&draw.constants, .offset = 0, .count = 4});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = draw.instance_count,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = draw.start_instance,
												});
		}

		backend->cmd_end_render_pass(cmd_buffer, {});

		barriers.resize(0);
		barriers.push_back({
			.resource	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_depth_write,
			.to_states	 = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_pre_depth::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_pre_depth::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_texture(pfd.depth_texture);
		}
	}

	void render_pass_pre_depth::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.depth_texture = backend->create_texture({
				.texture_format		  = render_target_definitions::get_format_depth_default_read(),
				.depth_stencil_format = render_target_definitions::get_format_depth_default(),
				.size				  = sz,
				.flags				  = texture_flags::tf_depth_texture | texture_flags::tf_typeless | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views				  = {{.type = view_type::depth_stencil}, {.type = view_type::depth_stencil, .read_only = 1}, {.type = view_type::sampled}},
				.clear_values		  = {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name			  = "prepass_depth",
			});
		}
	}
}
