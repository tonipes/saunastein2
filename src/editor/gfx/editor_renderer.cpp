// Copyright (c) 2025 Inan Evin

#include "editor_renderer.hpp"
#include "project/engine_data.hpp"
#include "data/vector_util.hpp"

// math
#include "math/vector2.hpp"
#include "math/vector4.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/common/render_target_definitions.hpp"

#include "gui/vekt.hpp"

namespace SFG
{
	void editor_renderer::init(texture_queue* texture_queue, const vector2ui16& screen_size)
	{
		_gfx_data.texture_queue = texture_queue;
		_gfx_data.window_size	= screen_size;

		create_textures(screen_size);

		// Shaders
		_shaders.gui_default = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_default).get_hw();
		_shaders.gui_sdf	 = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_sdf).get_hw();
		_shaders.gui_text	 = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_text).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_type_gui_default)
			{
				_shaders.gui_default = sh.get_hw();
				return;
			}
			if (type == engine_shader_type::engine_shader_type_gui_text)
			{
				_shaders.gui_text = sh.get_hw();
				return;
			}
			if (type == engine_shader_type::engine_shader_type_gui_sdf)
			{
				_shaders.gui_sdf = sh.get_hw();
				return;
			}
		});
#endif

		gfx_backend* backend = gfx_backend::get();
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.buf_pass_data.create_hw({.size = sizeof(gui_pass_view), .flags = resource_flags::rf_cpu_visible | resource_flags::rf_constant_buffer, .debug_name = "cbv_editor_gui_pass"});
			pfd.buf_gui_vtx.create_staging_hw({.size = sizeof(vekt::vertex) * 240000, .flags = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible, .debug_name = "editor_gui_vertex_stg"},
											  {.size = sizeof(vekt::vertex) * 240000, .flags = resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only, .debug_name = "editor_gui_vertex_gpu"});
			pfd.buf_gui_idx.create_staging_hw({.size = sizeof(vekt::index) * 320000, .flags = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible, .debug_name = "editor_gui_index_stg"},
											  {.size = sizeof(vekt::index) * 320000, .flags = resource_flags::rf_index_buffer | resource_flags::rf_gpu_only, .debug_name = "editor_gui_index_gpu"});
		}

		_builder	  = new vekt::builder();
		_font_manager = new vekt::font_manager();
		_builder->init({
			.vertex_buffer_sz			 = 1024 * 1024 * 4,
			.index_buffer_sz			 = 1024 * 1024 * 8,
			.text_cache_vertex_buffer_sz = 1024 * 1024 * 2,
			.text_cache_index_buffer_sz	 = 1024 * 1024 * 4,
			.buffer_count				 = 3,
		});

		_builder->set_on_draw(on_draw, this);
		_font_manager->init();
		_font_manager->set_callback_user_data(this);
		_font_manager->set_atlas_created_callback(on_atlas_created);
		_font_manager->set_atlas_updated_callback(on_atlas_updated);
		_font_manager->set_atlas_destroyed_callback(on_atlas_destroyed);

#ifdef SFG_TOOLMODE
		const string p = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		_font_manager->set_callback_user_data(this);
		_font_main = _font_manager->load_font_from_file(p.c_str(), 18);
#else
		SFG_NOTIMPLEMENTED();
#endif

		// gui
		_gui_world_overlays.init(_builder);
	}

	void editor_renderer::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		_font_manager->unload_font(_font_main);
		_font_manager->uninit();
		delete _font_manager;
		_font_manager = nullptr;

		_builder->uninit();
		delete _builder;
		_builder = nullptr;

		for (auto& ref : _gfx_data.atlases)
		{
			backend->destroy_texture(ref.texture);
			backend->destroy_resource(ref.intermediate_buffer);
		}
		_gfx_data.atlases.clear();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.buf_pass_data.destroy();
			pfd.buf_gui_vtx.destroy();
			pfd.buf_gui_idx.destroy();
		}

		destroy_textures();
	}

	void editor_renderer::prepare(gfx_id cmd_buffer, uint8 frame_index)
	{
		gfx_backend* backend  = gfx_backend::get();
		_gfx_data.frame_index = frame_index;

		per_frame_data& pfd = _pfd[frame_index];
		pfd.reset();

		// -----------------------------------------------------------------------------
		// render pass data
		// -----------------------------------------------------------------------------

		const gui_pass_view view = {
			.proj		   = matrix4x4::ortho_reverse_z(0, static_cast<float>(_gfx_data.window_size.x), 0, static_cast<float>(_gfx_data.window_size.y), 0.0f, 1.0f),
			.sdf_thickness = 0.5f,
			.sdf_softness  = 0.02f,
		};
		pfd.buf_pass_data.buffer_data(0, (void*)&view, sizeof(gui_pass_view));

		// -----------------------------------------------------------------------------
		// flush commands and draw gui here.
		// -----------------------------------------------------------------------------

		_builder->build_begin(vector2(_gfx_data.window_size.x, _gfx_data.window_size.y));

		_gui_world_overlays.draw(_builder);

		_builder->build_end();
		_builder->flush();

		// -----------------------------------------------------------------------------
		// copy vtx-index
		// -----------------------------------------------------------------------------
		{
			static_vector<barrier, 2> barriers;
			barriers.push_back({
				.resource	 = pfd.buf_gui_idx.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
			});

			barriers.push_back({
				.resource	 = pfd.buf_gui_vtx.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
			});
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = 2});
			barriers.resize(0);

			// Copy from staging to GPU
			if (pfd.counter_vtx != 0)
				pfd.buf_gui_vtx.copy_region(cmd_buffer, 0, pfd.counter_vtx * sizeof(vekt::vertex));
			if (pfd.counter_idx != 0)
				pfd.buf_gui_idx.copy_region(cmd_buffer, 0, pfd.counter_idx * sizeof(vekt::index));

			barriers.push_back({
				.resource	 = pfd.buf_gui_idx.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
			});

			barriers.push_back({
				.resource	 = pfd.buf_gui_vtx.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
			});

			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = 2});
		}
	}

	void editor_renderer::render(const render_params& p)
	{
		gfx_backend*	backend = gfx_backend::get();
		per_frame_data& pfd		= _pfd[p.frame_index];

		const gfx_id cmd_buffer			 = p.cmd_buffer;
		const gfx_id render_target		 = pfd.hw_rt;
		const gfx_id gui_vertex			 = pfd.buf_gui_vtx.get_hw_gpu();
		const gfx_id gui_index			 = pfd.buf_gui_idx.get_hw_gpu();
		const uint16 dc_count			 = pfd.draw_call_count;
		const uint32 gpu_index_pass_data = pfd.buf_pass_data.get_gpu_index();

		static_vector<barrier, 1> barriers;

		// in barrier
		{
			barriers.push_back({
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_texture,
				.from_states = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
				.to_states	 = resource_state::resource_state_render_target,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
		}

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "editor_pass");

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 0.0f),
			.texture	 = render_target,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = &att,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		static_vector<uint32, 4> rp_constants;
		rp_constants.push_back(gpu_index_pass_data);

		// Bind GUI pass constants (CBV index)
		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)rp_constants.data(), .offset = constant_index_rp_constant0, .count = static_cast<uint8>(rp_constants.size()), .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),

								  });

		// Vertex/index buffers
		backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = gui_vertex, .vertex_size = sizeof(vekt::vertex)});
		backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = gui_index, .index_size = sizeof(vekt::index)});

		gfx_id last_pipeline	   = NULL_GFX_ID;
		uint32 last_atlas_constant = UINT32_MAX;

		for (uint16 i = 0; i < dc_count; i++)
		{
			gui_draw_call& dc = _gui_draw_calls[i];

			backend->cmd_set_scissors(cmd_buffer, {.x = dc.scissors.x, .y = dc.scissors.y, .width = dc.scissors.z, .height = dc.scissors.w});

			if (last_pipeline == NULL_GFX_ID || last_pipeline != dc.shader)
			{
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = dc.shader});
				last_pipeline = dc.shader;
			}

			if (last_atlas_constant == UINT32_MAX || (dc.atlas_gpu_index != last_atlas_constant))
			{
				backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&dc.atlas_gpu_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});
				last_atlas_constant = dc.atlas_gpu_index;
			}

			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = dc.index_count,
													.instance_count			  = 1,
													.start_index_location	  = dc.start_idx,
													.base_vertex_location	  = dc.start_vtx,
													.start_instance_location  = 0,
												});
		}

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		// out barrier
		{
			barriers.resize(0);
			barriers.push_back({
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_texture,
				.from_states = resource_state::resource_state_render_target,
				.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
		}
	}

	void editor_renderer::resize(const vector2ui16& size)
	{
		if (_gfx_data.window_size == size)
			return;

		_gfx_data.window_size = size;

		destroy_textures();
		create_textures(size);
	}

	void editor_renderer::create_textures(const vector2ui16& size)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.hw_rt = backend->create_texture({
				.texture_format = render_target_definitions::get_format_editor(),
				.size			= size,
				.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 0.0f},
				.debug_name		= "editor_rt",
			});

			pfd.gpu_index_rt = backend->get_texture_gpu_index(pfd.hw_rt, 1);
		}
	}

	void editor_renderer::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.hw_rt);
			pfd.gpu_index_rt = NULL_GPU_INDEX;
		}
	}

	void editor_renderer::on_draw(const vekt::draw_buffer& buffer, void* ud)
	{
		editor_renderer* rnd = static_cast<editor_renderer*>(ud);

		const vekt::font*	  font			   = buffer.used_font;
		const vekt::atlas*	  atlas			   = font ? font->_atlas : nullptr;
		const vekt::font_type font_type		   = font ? font->type : vekt::font_type::normal;
		const vekt::vertex*	  buffer_vtx_start = buffer.vertex_start;
		const vekt::index*	  buffer_idx_start = buffer.index_start;
		const vector4		  clip			   = buffer.clip;
		const uint32		  buffer_idx_count = buffer.index_count;
		const uint32		  buffer_vtx_count = buffer.vertex_count;

		const gfx_id sdf_shader		= rnd->_shaders.gui_sdf;
		const gfx_id text_shader	= rnd->_shaders.gui_text;
		const gfx_id default_shader = rnd->_shaders.gui_default;

		per_frame_data& pfd			= rnd->_pfd[rnd->_gfx_data.frame_index];
		const uint32	vtx_counter = pfd.counter_vtx;
		const uint32	idx_counter = pfd.counter_idx;
		const uint16	dc_index	= pfd.draw_call_count++;
		pfd.counter_vtx += buffer_vtx_count;
		pfd.counter_idx += buffer_idx_count;
		pfd.buf_gui_vtx.buffer_data(sizeof(vekt::vertex) * static_cast<size_t>(vtx_counter), buffer_vtx_start, static_cast<size_t>(buffer_vtx_count) * sizeof(vekt::vertex));
		pfd.buf_gui_idx.buffer_data(sizeof(vekt::index) * static_cast<size_t>(idx_counter), buffer_idx_start, static_cast<size_t>(buffer_idx_count) * sizeof(vekt::index));

		gui_draw_call& dc = rnd->_gui_draw_calls[dc_index];
		dc				  = {};
		dc.start_idx	  = static_cast<uint16>(idx_counter);
		dc.start_vtx	  = static_cast<uint16>(vtx_counter);
		dc.index_count	  = static_cast<uint16>(buffer_idx_count);

		// Scissor
		if (clip.y < 0.0f)
		{
			dc.scissors.y = 0;
			dc.scissors.w = static_cast<uint16>(math::max(0.0f, clip.w + clip.y));
		}
		else
		{
			dc.scissors.y = static_cast<uint16>(clip.y);
			dc.scissors.w = static_cast<uint16>(clip.w);
		}
		if (clip.x < 0.0f)
		{
			dc.scissors.x = 0;
			dc.scissors.z = math::min((uint16)0, static_cast<uint16>(clip.x + clip.z));
		}
		else
		{
			dc.scissors.x = static_cast<uint16>(clip.x);
			dc.scissors.z = static_cast<uint16>(clip.z);
		}

		if (font)
		{
			dc.shader = font_type == vekt::font_type::sdf ? sdf_shader : text_shader;

			bool found = false;
			for (atlas_ref& ref : rnd->_gfx_data.atlases)
			{
				if (ref.atlas == atlas)
				{
					dc.atlas_gpu_index = ref.texture_gpu_index;
					found			   = true;
					break;
				}
			}

			SFG_ASSERT(found);
		}
		else
		{
			dc.shader = default_shader;
		}
	}

	void editor_renderer::on_atlas_created(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();
		engine_shaders&	 es		 = engine_shaders::get();

		atlas_ref ref		  = {};
		ref.atlas			  = atlas;
		ref.texture			  = backend->create_texture({.texture_format = atlas->get_is_lcd() ? format::r8g8b8a8_srgb : format::r8_unorm,
														 .size			 = vector2ui16(static_cast<uint16>(atlas->get_width()), static_cast<uint16>(atlas->get_height())),
														 .flags			 = texture_flags::tf_sampled | texture_flags::tf_is_2d,
														 .views			 = {{.type = view_type::sampled}},
														 .debug_name	 = "editor_gui_atlas"});
		ref.texture_gpu_index = backend->get_texture_gpu_index(ref.texture, 0);

		const uint32 txt_size	   = backend->get_texture_size(atlas->get_width(), atlas->get_height(), atlas->get_is_lcd() ? 3 : 1);
		const uint32 adjusted_size = backend->align_texture_size(txt_size);
		ref.intermediate_buffer	   = backend->create_resource({
			   .size	   = adjusted_size,
			   .flags	   = resource_flags::rf_cpu_visible,
			   .debug_name = "editor_gui_inter_buffer",
		   });

		r->_gfx_data.atlases.push_back(ref);
	}

	void editor_renderer::on_atlas_updated(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();

		for (atlas_ref& ref : r->_gfx_data.atlases)
		{
			if (ref.atlas == atlas)
			{
				const unsigned char* data		  = atlas->get_data();
				const unsigned int	 size		  = atlas->get_data_size();
				const bool			 is_lcd		  = atlas->get_is_lcd();
				const uint32		 atlas_width  = atlas->get_width();
				const uint32		 atlas_height = atlas->get_height();
				const uint8			 bpp		  = atlas->get_is_lcd() ? 3 : 1;

				uint32 adjusted_size = 0;
				ref.buffer.pixels	 = reinterpret_cast<uint8*>(backend->adjust_buffer_pitch((void*)data, atlas_width, atlas_height, bpp, adjusted_size));
				ref.buffer.size		 = vector2ui16(static_cast<uint16>(atlas_width), static_cast<uint16>(atlas_height));
				ref.buffer.bpp		 = bpp;

				static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
				buffers.push_back(ref.buffer);
				r->_gfx_data.texture_queue->add_request(buffers, ref.texture, ref.intermediate_buffer, 0, resource_state::resource_state_ps_resource);
				return;
			}
		}
	}

	void editor_renderer::on_atlas_destroyed(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();

		for (atlas_ref& ref : r->_gfx_data.atlases)
		{
			if (ref.atlas == atlas)
			{
				backend->destroy_texture(ref.texture);
				backend->destroy_resource(ref.intermediate_buffer);
				r->_gfx_data.atlases.remove(ref);
				return;
			}
		}

		SFG_ASSERT(false);
	}
}
