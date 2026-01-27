#include "render_pass_sprite.hpp"
#include "game/game_max_defines.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

	void render_pass_sprite::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_SPRITE, 8);
		_reuse_groups.reserve(MAX_DRAW_CALLS_SPRITE);

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.cmd_buffer		= backend->create_command_buffer({.type = command_type::graphics, .debug_name = "sprite_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "sprite_ubo"});
			pfd.instance_data.create(
				{
					.size		= sizeof(sprite_instance_data) * MAX_WORLD_COMP_SPRITES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "sprite_instances",
				},
				{
					.size			 = sizeof(sprite_instance_data) * MAX_WORLD_COMP_SPRITES,
					.structure_size	 = sizeof(sprite_instance_data),
					.structure_count = MAX_WORLD_COMP_SPRITES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only,
					.debug_name		 = "sprite_instances_gpu",
				});
		}
	}

	void render_pass_sprite::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
			pfd.instance_data.destroy();
		}
	}

	void render_pass_sprite::prepare(uint8 frame_index, proxy_manager& pm, const view& main_camera_view)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_SPRITE);

		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		gfx_backend* backend = gfx_backend::get();
		backend->reset_command_buffer(cmd_buffer);

		auto*		 sprites	  = pm.get_sprites();
		auto*		 materials	  = pm.get_material_runtimes();
		auto*		 entities	  = pm.get_entities();
		const uint32 peak_sprites = pm.get_peak_sprites();

		_reuse_groups;
		_reuse_groups.resize(0);
		_instance_count = 0;

		for (uint32 i = 0; i < peak_sprites; i++)
		{
			render_proxy_sprite& sprite = sprites->get(i);
			if (sprite.status != render_proxy_status::rps_active)
				continue;

			if (sprite.material == NULL_RESOURCE_ID)
				continue;

			render_proxy_entity& entity = entities->get(sprite.entity);
			if (entity.status != render_proxy_status::rps_active)
				continue;

			if (entity._assigned_index == UINT32_MAX)
				continue;

			render_proxy_material_runtime& mat = materials->get(sprite.material);
			if (!mat.flags.is_set(material_flags::material_flags_is_sprite))
				continue;
			if (mat.shader_handle == NULL_RESOURCE_ID)
				continue;

			const gfx_id pipeline = pm.get_shader_variant(mat.shader_handle, 0);
			if (pipeline == NULL_GFX_ID)
				continue;

			int32 group_index = -1;
			for (uint32 g = 0; g < _reuse_groups.size(); g++)
			{
				sprite_group& group = _reuse_groups[g];
				if (group.pipeline == pipeline && group.material == sprite.material)
				{
					group_index = static_cast<int32>(g);
					break;
				}
			}

			if (group_index == -1)
			{
				_reuse_groups.push_back({
					.pipeline = pipeline,
					.material = sprite.material,
					.start	  = 0,
					.count	  = 0,
					.cursor	  = 0,
				});
				group_index = static_cast<int32>(_reuse_groups.size() - 1);
			}

			_reuse_groups[group_index].count++;
			_instance_count++;
		}

		uint32 offset = 0;
		for (uint32 g = 0; g < _reuse_groups.size(); g++)
		{
			_reuse_groups[g].start	= offset;
			_reuse_groups[g].cursor = 0;
			offset += _reuse_groups[g].count;
		}

		if (_instance_count != 0)
		{
			sprite_instance_data* instance_data = reinterpret_cast<sprite_instance_data*>(pfd.instance_data.get_mapped());

			for (uint32 i = 0; i < peak_sprites; i++)
			{
				render_proxy_sprite& sprite = sprites->get(i);
				if (sprite.status != render_proxy_status::rps_active)
					continue;

				if (sprite.material == NULL_RESOURCE_ID)
					continue;

				render_proxy_entity& entity = entities->get(sprite.entity);
				if (entity.status != render_proxy_status::rps_active)
					continue;

				if (entity._assigned_index == UINT32_MAX)
					continue;

				render_proxy_material_runtime& mat = materials->get(sprite.material);
				if (!mat.flags.is_set(material_flags::material_flags_is_sprite))
					continue;
				if (mat.shader_handle == NULL_RESOURCE_ID)
					continue;

				const gfx_id pipeline = pm.get_shader_variant(mat.shader_handle, 0);
				if (pipeline == NULL_GFX_ID)
					continue;

				int32 group_index = -1;
				for (uint32 g = 0; g < _reuse_groups.size(); g++)
				{
					sprite_group& group = _reuse_groups[g];
					if (group.pipeline == pipeline && group.material == sprite.material)
					{
						group_index = static_cast<int32>(g);
						break;
					}
				}

				if (group_index == -1)
					continue;

				sprite_group& group			= _reuse_groups[group_index];
				const uint32  target_index	= group.start + group.cursor;
				instance_data[target_index] = {
					.entity_index = entity._assigned_index,
				};
				group.cursor++;
			}

			static_vector<barrier, 1> barriers;
			barriers.push_back({
				.from_states = resource_state::resource_state_non_ps_resource,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.instance_data.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

			pfd.instance_data.copy_region(cmd_buffer, 0, sizeof(sprite_instance_data) * _instance_count);

			barriers[0] = {
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = pfd.instance_data.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			};
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		}

		for (uint32 g = 0; g < _reuse_groups.size(); g++)
		{
			sprite_group&				   group = _reuse_groups[g];
			render_proxy_material_runtime& mat	 = materials->get(group.material);

			_draw_stream.add_command({
				.start_instance			 = group.start,
				.instance_count			 = group.count,
				.material_constant_index = mat.gpu_index_buffers[frame_index],
				.texture_constant_index	 = mat.gpu_index_texture_buffers[frame_index],
				.sampler_constant_index	 = mat.gpu_index_sampler,
				.pipeline_hw			 = group.pipeline,
			});
		}

		const ubo ubo_data = {
			.view_proj = main_camera_view.view_proj_matrix,
		};
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_sprite::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		if (_instance_count == 0)
		{
			backend->close_command_buffer(cmd_buffer);
			return;
		}

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = p.input_texture,
			.load_op	 = load_op::load,
			.store_op	 = store_op::store,
		};

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "sprite_pass");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = &att,
														   .depth_stencil_attachment =
															   {
																   .texture		   = p.depth_texture,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = 1,
													   });

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),
								  });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const uint32 rp_constants[3] = {pfd.ubo.get_index(), p.gpu_index_entities, pfd.instance_data.get_index()};
		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)rp_constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});

		_draw_stream.build();
		_draw_stream.draw(cmd_buffer);

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->close_command_buffer(cmd_buffer);
	}
}
