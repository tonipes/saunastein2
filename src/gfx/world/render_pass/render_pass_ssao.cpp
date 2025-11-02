// Copyright (c) 2025 Inan Evin

#include "render_pass_ssao.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"

namespace SFG
{
	void render_pass_ssao::init(const vector2ui16& size)
	{
		_alloc.init(1024, 8);

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::compute, .debug_name = "ssao_cmd"});
			pfd.ubo.create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "ssao_ubo"});
		}

		create_textures(size);

		_shader_ssao = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_hbao).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_type_hbao)
			{
				_shader_ssao = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_ssao::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
		}

		destroy_textures();
	}

	void render_pass_ssao::prepare(proxy_manager& pm, const view& camera_view, uint8 frame_index)
	{
		_alloc.reset();

		const uint8					ambient_exists = pm.get_ambient_exists();
		const render_proxy_ambient& ambient		   = pm.get_ambient();
		const vector3				ambient_color  = ambient_exists ? ambient.base_color : vector3(0.1f, 0.1f, 0.1f);

		per_frame_data& pfd = _pfd[frame_index];

		const ubo ubo_data = {
			.inverse_view_proj			 = camera_view.view_proj_matrix.inverse(),
			.ambient_color_plights_count = vector4(ambient_color.x, ambient_color.y, ambient_color.z, static_cast<float>(pm.get_count_point_lights())),
			.view_position_slights_count = vector4(camera_view.position.x, camera_view.position.y, camera_view.position.z, static_cast<float>(pm.get_count_spot_lights())),
			.dir_lights_count			 = pm.get_count_dir_lights(),
			.cascade_levels_gpu_index	 = camera_view.cascsades_gpu_index,
			.cascade_count				 = static_cast<uint32>(camera_view.cascades.size()),
			.near_plane					 = camera_view.near_plane,
			.far_plane					 = camera_view.far_plane,
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_ssao::render(const render_params& p)
	{
		return;
		/*
		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	queue_gfx		 = backend->get_queue_gfx();
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gfx_id	color_texture	 = pfd.render_target;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_gpu_index();
		const gfx_id	sh				 = _shader_ssao;

		// RP constants.
		static_vector<gpu_index, GBUFFER_COLOR_TEXTURES + 10> rp_constants;
		rp_constants.push_back(p.gpu_index_entities);
		rp_constants.push_back(p.gpu_index_point_lights);
		rp_constants.push_back(p.gpu_index_spot_lights);
		rp_constants.push_back(p.gpu_index_dir_lights);
		rp_constants.push_back(p.gpu_index_shadow_data_buffer);
		rp_constants.push_back(p.gpu_index_float_buffer);
		for (uint32 i = 0; i < GBUFFER_COLOR_TEXTURES; i++)
			rp_constants.push_back(p.gpu_index_gbuffer_textures[i]);
		rp_constants.push_back(p.gpu_index_depth_texture);
		SFG_ASSERT(rp_constants.size() < constant_index_object_constant0 - constant_index_rp_constant0 + 1);

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.resource	 = color_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_render_target,
		});

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "ssao_pass");

		render_pass_color_attachment* attachments = _alloc.allocate<render_pass_color_attachment>(1);
		render_pass_color_attachment& att		  = attachments[0];
		att.clear_color							  = vector4(0, 0, 0, 1.0f);
		att.load_op								  = load_op::clear;
		att.store_op							  = store_op::store;
		att.texture								  = color_texture;
		att.view_index							  = 0;

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = attachments,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&gpu_index_rp_ubo, .offset = constant_index_rp_ubo_index, .count = 1, .param_index = rpi_constants});
		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)rp_constants.data(), .offset = constant_index_rp_constant0, .count = static_cast<uint8>(rp_constants.size()), .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),

								  });
		backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = sh});
		backend->cmd_draw_instanced(cmd_buffer,
									{
										.vertex_count_per_instance = 3,
										.instance_count			   = 1,
									});

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.resize(0);

		barriers.push_back({
			.resource	 = color_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		barriers.push_back({
			.resource	 = p.depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_common,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
		*/
	}

	void render_pass_ssao::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_ssao::destroy_textures()
	{
		return;

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
		}
	}

	void render_pass_ssao::create_textures(const vector2ui16& sz)
	{
		return;

		gfx_backend* backend = gfx_backend::get();

		//	for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		//	{
		//		per_frame_data& pfd = _pfd[i];
		//
		//		pfd.render_target = backend->create_texture({
		//			.texture_format = render_target_definitions::get_format_ssao(),
		//			.size			= sz,
		//			.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
		//			.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
		//			.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
		//			.debug_name		= "ssao_rt",
		//		});
		//
		//		pfd.gpu_index_render_target = backend->get_texture_gpu_index(pfd.render_target, 1);
		//	}
	}
}
