// Copyright (c) 2025 Inan Evin

#include "render_pass_lighting.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/renderer.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "world/world.hpp"
#include "resources/vertex.hpp"
#include "math/vector2ui16.hpp"
#include "resources/shader_raw.hpp"

namespace SFG
{
	void render_pass_lighting::init(const init_params& params)
	{
		_alloc.init(params.alloc, params.alloc_size);

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "lighting_cmd"});
			pfd.ubo.create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "lighting_ubo"});

			pfd.gpu_index_entity_buffer		 = params.entities[i];
			pfd.gpu_index_point_light_buffer = params.point_lights[i];
			pfd.gpu_index_spot_light_buffer	 = params.spot_lights[i];
			pfd.gpu_index_dir_light_buffer	 = params.dir_lights[i];
		}

		create_textures(params.size, params.gbuffer_textures, params.depth_textures, params.depth_textures_hw);

		_shader_lighting = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_world_lighting).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_type_world_lighting)
			{
				_shader_lighting = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_lighting::uninit()
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

	void render_pass_lighting::prepare(proxy_manager& pm, const renderable_collector& collector, uint8 frame_index)
	{
		const uint8					ambient_exists = pm.get_ambient_exists();
		const render_proxy_ambient& ambient		   = pm.get_ambient();
		const vector3				ambient_color  = ambient_exists ? ambient.base_color : vector3(0.1f, 0.1f, 0.1f);

		per_frame_data& pfd			= _pfd[frame_index];
		const view&		camera_view = collector.get_view();

		const ubo ubo_data = {
			.inverse_view_proj			 = camera_view.view_proj_matrix.inverse(),
			.ambient_color_plights_count = vector4(ambient_color.x, ambient_color.y, ambient_color.z, static_cast<float>(pm.get_count_point_lights())),
			.view_position_slights_count = vector4(camera_view.position.x, camera_view.position.y, camera_view.position.z, static_cast<float>(pm.get_count_spot_lights())),
			.dir_lights_count			 = static_cast<float>(pm.get_count_dir_lights()),
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_lighting::render(const render_params& p)
	{
		gfx_backend*	backend					  = gfx_backend::get();
		per_frame_data& pfd						  = _pfd[p.frame_index];
		const gfx_id	queue_gfx				  = backend->get_queue_gfx();
		const gfx_id	cmd_buffer				  = pfd.cmd_buffer;
		const gfx_id	color_texture			  = pfd.render_target;
		const gfx_id	depth_texture			  = pfd.depth_texture;
		const gpu_index gpu_index_rp_ubo		  = pfd.ubo.get_gpu_heap_index();
		const gpu_index gpu_index_rp_entities	  = pfd.gpu_index_entity_buffer;
		const gpu_index gpu_index_rp_point_lights = pfd.gpu_index_point_light_buffer;
		const gpu_index gpu_index_rp_spot_lights  = pfd.gpu_index_spot_light_buffer;
		const gpu_index gpu_index_rp_dir_lights	  = pfd.gpu_index_dir_light_buffer;
		const gfx_id	sh						  = _shader_lighting;

		// RP constants.
		static_vector<gpu_index, GBUFFER_COLOR_TEXTURES + 10> rp_constants;
		rp_constants.push_back(gpu_index_rp_entities);
		rp_constants.push_back(gpu_index_rp_point_lights);
		rp_constants.push_back(gpu_index_rp_spot_lights);
		rp_constants.push_back(gpu_index_rp_dir_lights);
		for (uint32 i = 0; i < GBUFFER_COLOR_TEXTURES; i++)
			rp_constants.push_back(pfd.gpu_index_gbuffer_textures[i]);
		rp_constants.push_back(pfd.gpu_index_depth_texture);
		SFG_ASSERT(rp_constants.size() < constant_index_object_constant0 - constant_index_rp_constant0);

		_alloc.reset();

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

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "lighting_pass");

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
			.resource	 = depth_texture,
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
	}

	void render_pass_lighting::resize(const vector2ui16& size, gpu_index* gbuffer_textures, gpu_index* depth_textures, gfx_id* depth_textures_hw)
	{
		destroy_textures();
		create_textures(size, gbuffer_textures, depth_textures, depth_textures_hw);
	}

	void render_pass_lighting::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
		}
	}

	void render_pass_lighting::create_textures(const vector2ui16& sz, gpu_index* gbuffer_textures, gpu_index* depth_textures, gfx_id* depth_textures_hw)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.render_target = backend->create_texture({
				.texture_format = render_target_definitions::get_format_lighting(),
				.size			= sz,
				.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "lighting_rt",
			});

			pfd.gpu_index_render_target = backend->get_texture_gpu_index(pfd.render_target, 1);

			const uint32 base = i * GBUFFER_COLOR_TEXTURES;

			for (uint8 j = 0; j < GBUFFER_COLOR_TEXTURES; j++)
			{
				pfd.gpu_index_gbuffer_textures[j] = gbuffer_textures[base + j];
			}
			pfd.gpu_index_depth_texture = depth_textures[i];
			pfd.depth_texture			= depth_textures_hw[i];
		}
	}
}
