// Copyright (c) 2025 Inan Evin

#include "render_pass_lighting_forward.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"

#include "gfx/world/gpu_bone.hpp"
#include "gfx/world/gpu_entity.hpp"
#include "gfx/world/gpu_light.hpp"

namespace SFG
{
	void render_pass_lighting_forward::init(const init_data& data)
	{
		_alloc.init(data.alloc, data.alloc_size);
		gfx_backend* backend = gfx_backend::get();

		create_textures(data.size);

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer			= backend->create_command_buffer({.type = command_type::graphics, .debug_name = "opaque_cmd"});
			pfd.semaphore.semaphore = backend->create_semaphore();

			pfd.ubo_lighting.create_hw({.size = sizeof(ubo_lighting), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "lighting_ubo"});
			pfd.ubo_forward.create_hw({.size = sizeof(ubo_forward), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "forward_ubo"});

			pfd.bones.create_staging_hw(
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_bones_cpu",
				},
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "opaque_bones_gpu",
				});

			pfd.entities.create_staging_hw(
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_entities_cpu",
				},
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "opaque_entities_gpu",
				});

			pfd.dir_lights.create_staging_hw(
				{
					.size		= sizeof(gpu_dir_light) * MAX_GPU_DIR_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_dir_lights_cpu",
				},
				{
					.size		= sizeof(gpu_dir_light) * MAX_GPU_DIR_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_dir_lights_gpu",
				});

			pfd.point_lights.create_staging_hw(
				{
					.size		= sizeof(gpu_point_light) * MAX_GPU_POINT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_point_lights_cpu",
				},
				{
					.size		= sizeof(gpu_point_light) * MAX_GPU_POINT_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_point_lights_gpu",
				});

			pfd.spot_lights.create_staging_hw(
				{
					.size		= sizeof(gpu_spot_light) * MAX_GPU_SPOT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_spot_lights_cpu",
				},
				{
					.size		= sizeof(gpu_spot_light) * MAX_GPU_SPOT_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_spot_lights_cpu",
				});

			const uint32 base		= i == 0 ? 0 : 4;
			pfd.bind_group_lighting = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(pfd.bind_group_lighting, rpi_table_render_pass, 10, false);
			backend->bind_group_update_pointer(pfd.bind_group_lighting,
											   0,
											   {
												   {.resource = pfd.ubo_lighting.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
												   {.resource = pfd.bones.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo0, .type = binding_type::ssbo},
												   {.resource = pfd.dir_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo1, .type = binding_type::ssbo},
												   {.resource = pfd.point_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo2, .type = binding_type::ssbo},
												   {.resource = pfd.spot_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo3, .type = binding_type::ssbo},
												   {.resource = data.opaque_textures[base], .view = 0, .pointer_index = upi_render_pass_texture0, .type = binding_type::texture_binding},
												   {.resource = data.opaque_textures[base + 1], .view = 0, .pointer_index = upi_render_pass_texture1, .type = binding_type::texture_binding},
												   {.resource = data.opaque_textures[base + 2], .view = 0, .pointer_index = upi_render_pass_texture2, .type = binding_type::texture_binding},
												   {.resource = data.opaque_textures[base + 3], .view = 0, .pointer_index = upi_render_pass_texture3, .type = binding_type::texture_binding},
												   {.resource = data.depth_textures[i], .view = 0, .pointer_index = upi_render_pass_texture4, .type = binding_type::texture_binding},
											   });

			pfd.bind_group_forward = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(pfd.bind_group_forward, rpi_table_render_pass, 6, false);
			backend->bind_group_update_pointer(pfd.bind_group_forward,
											   0,
											   {
												   {.resource = pfd.ubo_forward.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
												   {.resource = pfd.entities.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo0, .type = binding_type::ssbo},
												   {.resource = pfd.bones.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo1, .type = binding_type::ssbo},
												   {.resource = pfd.dir_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo2, .type = binding_type::ssbo},
												   {.resource = pfd.point_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo3, .type = binding_type::ssbo},
												   {.resource = pfd.spot_lights.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ssbo4, .type = binding_type::ssbo},
											   });

			pfd.depth_texture = data.depth_textures[i];
		}
	}

	void render_pass_lighting_forward::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			backend->destroy_bind_group(pfd.bind_group_lighting);
			backend->destroy_bind_group(pfd.bind_group_forward);
			backend->destroy_semaphore(pfd.semaphore.semaphore);
			pfd.ubo_lighting.destroy();
			pfd.ubo_forward.destroy();
			pfd.dir_lights.destroy();
			pfd.spot_lights.destroy();
			pfd.point_lights.destroy();
		}

		destroy_textures();
	}

	void render_pass_lighting_forward::reset_target_textures(gfx_id* opaque_textures, gfx_id* depth_textures)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.depth_texture	= depth_textures[i];
			const uint32 base	= i == 0 ? 0 : 4;

			backend->bind_group_update_pointer(pfd.bind_group_lighting,
											   0,
											   {
												   {.resource = opaque_textures[base], .view = 0, .pointer_index = upi_render_pass_texture0, .type = binding_type::texture_binding},
												   {.resource = opaque_textures[base + 1], .view = 0, .pointer_index = upi_render_pass_texture1, .type = binding_type::texture_binding},
												   {.resource = opaque_textures[base + 2], .view = 0, .pointer_index = upi_render_pass_texture2, .type = binding_type::texture_binding},
												   {.resource = opaque_textures[base + 3], .view = 0, .pointer_index = upi_render_pass_texture3, .type = binding_type::texture_binding},
												   {.resource = opaque_textures[i], .view = 0, .pointer_index = upi_render_pass_texture4, .type = binding_type::texture_binding},
											   });
		}
	}

	void render_pass_lighting_forward::render(uint8 frame_index, const vector2ui16& size, gfx_id global_layout, gfx_id global_group)
	{
		gfx_backend*	backend				   = gfx_backend::get();
		per_frame_data& pfd					   = _pfd[frame_index];
		render_data&	rd					   = _render_data;
		const gfx_id*	textures			   = pfd.color_textures.data();
		const gfx_id	cmd_buffer			   = pfd.cmd_buffer;
		const gfx_id	depth_texture		   = pfd.depth_texture;
		const gfx_id	rp_bind_group_lighting = pfd.bind_group_lighting;
		const gfx_id	rp_bind_group_forward  = pfd.bind_group_forward;

		render_pass_color_attachment* attachments = _alloc.allocate<render_pass_color_attachment>(COLOR_TEXTURES);

		for (uint8 i = 0; i < COLOR_TEXTURES; i++)
		{
			render_pass_color_attachment& att = attachments[i];
			att.clear_color					  = vector4(1, 0, 0, 1.0f);
			att.load_op						  = load_op::clear;
			att.store_op					  = store_op::store;
			att.texture						  = textures[i];
		}

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = attachments,
										   .color_attachment_count = COLOR_TEXTURES,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = global_group});
		backend->cmd_bind_group(cmd_buffer, {.group = rp_bind_group_lighting});
		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});
		backend->cmd_set_viewport(cmd_buffer, {.min_depth = 0.0f, .max_depth = 0.0f, .width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});

		backend->cmd_draw_instanced(cmd_buffer,
									{
										.vertex_count_per_instance = 4,
									});
		backend->cmd_end_render_pass(cmd_buffer, {});
	}

	void render_pass_lighting_forward::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_lighting_forward::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			for (uint32 i = 0; i < COLOR_TEXTURES; i++)
				backend->destroy_texture(pfd.color_textures[i]);
			pfd.color_textures.clear();
		}
	}

	void render_pass_lighting_forward::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.color_textures.clear();

			for (uint32 j = 0; j < COLOR_TEXTURES; j++)
			{
				pfd.color_textures.push_back(backend->create_texture({
					.texture_format = format::r16g16b16a16_sfloat,
					.size			= sz,
					.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d,
				}));
			}
		}
	}
}
