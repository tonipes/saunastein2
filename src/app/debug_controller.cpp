/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "debug_controller.hpp"

// math
#include "math/vector2ui.hpp"
#include "math/math.hpp"
#include "math/color.hpp"

// data
#include "data/vector_util.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

// memory
#include "memory/memory.hpp"
#include "memory/bump_allocator.hpp"
#include "memory/memory_tracer.hpp"

// gfx
#include "gfx/buffer_queue.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/shared_cbv.hpp"

// misc
#include "common/system_info.hpp"
#include "serialization/serialization.hpp"
#include "input/input_mappings.hpp"
#include "debug_console.hpp"
#include "gui/vekt.hpp"

// platform
#include "platform/window_common.hpp"
#include "platform/process.hpp"

// io
#include "io/log.hpp"
#include "io/file_system.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

#define MAX_CONSOLE_TEXT		128
#define MAX_INPUT_FIELD			127
#define COLOR_TEXT				color::srgb_to_linear(color(129.0f / 255.0f, 220.0f / 255.0f, 148.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_TEXT_WARN			color::srgb_to_linear(color(240.0f / 255.0f, 220.0f / 255.0f, 148.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_TEXT_PROGRESS		color::srgb_to_linear(color(148.0f / 255.0f, 170.0f / 255.0f, 240.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_TEXT_ERR			color::srgb_to_linear(color(250.0f / 255.0f, 120.0f / 255.0f, 88.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_TEXT_DARK			color::srgb_to_linear(color(119.0f / 255.0f, 210.0f / 255.0f, 138.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_CONSOLE_BG		color::srgb_to_linear(color(12.0f / 255.0f, 16.0f / 255.0f, 12.0f / 255.0f, 0.95f)).to_vector()
#define COLOR_CONSOLE_BG_OPAQUE color::srgb_to_linear(color(12.0f / 255.0f, 16.0f / 255.0f, 12.0f / 255.0f, 1.0f)).to_vector()
#define COLOR_BORDER			color::srgb_to_linear(color(89.0f / 255.0f, 180.0f / 255.0f, 108.0f / 255.0f, 1.0f)).to_vector()
#define DEBUG_FONT_SIZE			20
#define INPUT_FIELD_HEIGHT		static_cast<float>(DEBUG_FONT_SIZE) * 1.5f
#define CONSOLE_SPACING			static_cast<float>(DEBUG_FONT_SIZE) * 0.5f
#define MAX_HISTORY				8
#define RT_FORMAT				format::r8g8b8a8_srgb
#define HISTORY_PATH			"console_history.stk"

	static constexpr float B_TO_MB = 1024.0f * 1024.0f;

	void debug_controller::build_console()
	{
		_input_field.text = _text_allocator.allocate(MAX_INPUT_FIELD);

		// vekt root
		{
			vekt::pos_props& pos_props = _vekt_data.builder->widget_get_pos_props(_vekt_data.builder->get_root());
			pos_props.flags			   = vekt::pos_flags::pf_child_pos_column;
		}

		// header
		{
			vekt::id header = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.builder->get_root(), header);

			_vekt_data.builder->widget_set_pos(header, vector2(0.0f, 0.0f));
			_vekt_data.builder->widget_set_size(header, vector2(1.0f, INPUT_FIELD_HEIGHT), vekt::helper_size_type::relative, vekt::helper_size_type::absolute);

			vekt::pos_props& pos_props = _vekt_data.builder->widget_get_pos_props(header);
			pos_props.flags			   = vekt::pos_flags::pf_child_pos_row;

			vekt::size_props& props	 = _vekt_data.builder->widget_get_size_props(header);
			props.child_margins.left = static_cast<float>(DEBUG_FONT_SIZE) * 0.5f;
			props.spacing			 = CONSOLE_SPACING;

			vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(header);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = COLOR_TEXT;

			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_text;
				gfx.color			  = COLOR_CONSOLE_BG_OPAQUE;

				vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
				tp.font				 = _vekt_data.font_debug;
				tp.text				 = _text_allocator.allocate("FPS: 1000");
				_vekt_data.builder->widget_update_text(w);

				_vekt_data.widget_fps = w;
			}

			// border
			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);
				_vekt_data.builder->widget_set_size(w, vector2(DEBUG_FONT_SIZE * 0.2f, 1.0f), vekt::helper_size_type::absolute, vekt::helper_size_type::relative);
				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
				gfx.color			  = COLOR_CONSOLE_BG;
			}

			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_text;
				gfx.color			  = COLOR_CONSOLE_BG_OPAQUE;

				vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
				tp.font				 = _vekt_data.font_debug;
				tp.text				 = _text_allocator.allocate("Main: 0.000000 ");
				_vekt_data.builder->widget_update_text(w);

				_vekt_data.widget_main_thread = w;
			}

			// border
			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);
				_vekt_data.builder->widget_set_size(w, vector2(DEBUG_FONT_SIZE * 0.2f, 1.0f), vekt::helper_size_type::absolute, vekt::helper_size_type::relative);
				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
				gfx.color			  = COLOR_CONSOLE_BG;
			}

			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_text;
				gfx.color			  = COLOR_CONSOLE_BG_OPAQUE;

				vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
				tp.font				 = _vekt_data.font_debug;
				tp.text				 = _text_allocator.allocate("Render: 0.000000 ");
				_vekt_data.builder->widget_update_text(w);

				_vekt_data.widget_render_thread = w;
			}

			// border
			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);
				_vekt_data.builder->widget_set_size(w, vector2(DEBUG_FONT_SIZE * 0.2f, 1.0f), vekt::helper_size_type::absolute, vekt::helper_size_type::relative);
				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
				gfx.color			  = COLOR_CONSOLE_BG;
			}

			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_text;
				gfx.color			  = COLOR_CONSOLE_BG_OPAQUE;

				vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
				tp.font				 = _vekt_data.font_debug;
				tp.text				 = _text_allocator.allocate("RAM: 0.00000");
				_vekt_data.builder->widget_update_text(w);

				_vekt_data.widget_global_mem = w;
			}

			// border
			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);
				_vekt_data.builder->widget_set_size(w, vector2(DEBUG_FONT_SIZE * 0.2f, 1.0f), vekt::helper_size_type::absolute, vekt::helper_size_type::relative);
				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
				gfx.color			  = COLOR_CONSOLE_BG;
			}

			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_text;
				gfx.color			  = COLOR_CONSOLE_BG_OPAQUE;

				vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
				tp.font				 = _vekt_data.font_debug;
				tp.text				 = _text_allocator.allocate("VRAM: 0.00000");
				_vekt_data.builder->widget_update_text(w);

				_vekt_data.widget_gfx_mem = w;
			}

			// border
			{
				vekt::id w = _vekt_data.builder->allocate();
				_vekt_data.builder->widget_add_child(header, w);
				_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);
				_vekt_data.builder->widget_set_size(w, vector2(DEBUG_FONT_SIZE * 0.2f, 1.0f), vekt::helper_size_type::absolute, vekt::helper_size_type::relative);
				vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
				gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
				gfx.color			  = COLOR_CONSOLE_BG;
			}
		}

		// Console parent
		{
			vekt::id w = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.builder->get_root(), w);
			_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.0f), vekt::helper_pos_type::relative, vekt::helper_pos_type::absolute);
			_vekt_data.builder->widget_set_size(w, vector2(1.0f, 1.0f), vekt::helper_size_type::relative, vekt::helper_size_type::fill);

			vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_clip_children;
			gfx.color			  = COLOR_CONSOLE_BG;

			vekt::pos_props& pos_props = _vekt_data.builder->widget_get_pos_props(w);
			pos_props.flags			   = vekt::pos_flags::pf_child_pos_column;

			vekt::size_props& props	   = _vekt_data.builder->widget_get_size_props(w);
			props.child_margins.left   = CONSOLE_SPACING;
			props.child_margins.top	   = CONSOLE_SPACING;
			props.child_margins.bottom = CONSOLE_SPACING;
			props.spacing			   = CONSOLE_SPACING;

			_vekt_data.widget_console_bg = w;
		}

		// border
		{
			vekt::id w = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.builder->get_root(), w);
			_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.0f), vekt::helper_pos_type::relative, vekt::helper_pos_type::absolute);
			_vekt_data.builder->widget_set_size(w, vector2(1.0f, DEBUG_FONT_SIZE * 0.05f), vekt::helper_size_type::relative, vekt::helper_size_type::absolute);
			vekt::widget_gfx& gfx	 = _vekt_data.builder->widget_get_gfx(w);
			gfx.flags				 = vekt::gfx_flags::gfx_is_rect;
			gfx.color				 = COLOR_BORDER;
			_vekt_data.widget_border = w;
		}

		// Input Field
		{
			vekt::id w = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.builder->get_root(), w);

			_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.0f));
			_vekt_data.builder->widget_set_size(w, vector2(1.0f, INPUT_FIELD_HEIGHT), vekt::helper_size_type::relative, vekt::helper_size_type::absolute);

			vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = COLOR_CONSOLE_BG;

			vekt::pos_props& pos_props = _vekt_data.builder->widget_get_pos_props(w);
			pos_props.flags			   = vekt::pos_flags::pf_child_pos_row;

			vekt::size_props& props		  = _vekt_data.builder->widget_get_size_props(w);
			props.child_margins.left	  = static_cast<float>(DEBUG_FONT_SIZE) * 0.5f;
			props.spacing				  = CONSOLE_SPACING;
			_vekt_data.widget_input_field = w;
		}

		// icon
		{
			vekt::id w = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.widget_input_field, w);
			_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

			vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = COLOR_TEXT;

			vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
			tp.font				 = _vekt_data.font_icon;
			tp.text				 = _text_allocator.allocate("\u0071");
			_vekt_data.builder->widget_update_text(w);
		}

		// input text
		{
			vekt::id w = _vekt_data.builder->allocate();
			_vekt_data.builder->widget_add_child(_vekt_data.widget_input_field, w);
			_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.5f), vekt::helper_pos_type::relative, vekt::helper_pos_type::relative, vekt::helper_anchor_type::start, vekt::helper_anchor_type::center);

			vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = COLOR_TEXT;

			vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
			tp.font				 = _vekt_data.font_debug;
			tp.text				 = _input_field.text;

			_vekt_data.widget_input_text = w;
			_vekt_data.builder->widget_update_text(w);
		}
		set_console_visible(false);
	}

	void debug_controller::init(texture_queue* texture_queue, gfx_id global_bind_layout, const vector2ui16& screen_size)
	{
		_text_allocator.init(100000);
		_gfx_data.texture_queue = texture_queue;
		_gfx_data.rt_size		= vector2ui16(screen_size.x, screen_size.y / 2);
		_gfx_data.screen_size	= vector2ui16(screen_size.x, screen_size.y);

		gfx_backend* backend = gfx_backend::get();

		_shaders.gui_default				   = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_default).get_hw();
		_shaders.gui_sdf					   = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_sdf).get_hw();
		_shaders.gui_text					   = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_gui_text).get_hw();
		_shaders.debug_controller_console_draw = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_debug_console).get_hw();

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

			if (type == engine_shader_type::engine_shader_type_debug_console)
			{
				_shaders.debug_controller_console_draw = sh.get_hw();
				return;
			}
		});
#endif

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.rt_console = backend->create_texture({
				.texture_format = RT_FORMAT,
				.size			= vector2ui16(screen_size.x, screen_size.y / 2),
				.flags			= texture_flags::tf_sampled | texture_flags::tf_is_2d | texture_flags::tf_render_target,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "console_rt",
			});

			pfd.rt_post = backend->create_texture({
				.texture_format = RT_FORMAT,
				.size			= vector2ui16(screen_size.x, screen_size.y / 2),
				.flags			= texture_flags::tf_sampled | texture_flags::tf_is_2d | texture_flags::tf_render_target,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "debug_rt",
			});

			pfd.rt_console_index	= backend->get_texture_gpu_index(pfd.rt_console, 1);
			pfd.rt_fullscreen_index = backend->get_texture_gpu_index(pfd.rt_post, 1);

			pfd.buf_pass_data.create({
				.size		= sizeof(gui_pass_view),
				.flags		= resource_flags::rf_cpu_visible | resource_flags::rf_constant_buffer,
				.debug_name = "cbv_gui_pass",
			});

			pfd.buf_gui_vtx.create(
				{
					.size		= sizeof(vekt::vertex) * 240000,
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible,
					.debug_name = "gui_vertex_stg",
				},
				{
					.size		= sizeof(vekt::vertex) * 240000,
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "gui_vertex_gpu",
				});

			pfd.buf_gui_idx.create(
				{
					.size		= sizeof(vekt::index) * 320000,
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible,
					.debug_name = "gui_index_stg",
				},
				{
					.size		= sizeof(vekt::index) * 320000,
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
					.debug_name = "gui_index_gpu",
				});
		}

		_vekt_data.builder		= new vekt::builder();
		_vekt_data.font_manager = new vekt::font_manager();
		_vekt_data.builder->init({
			.vertex_buffer_sz			 = 1024 * 1024 * 10,
			.index_buffer_sz			 = 1024 * 1024 * 20,
			.text_cache_vertex_buffer_sz = 1024 * 1024 * 10,
			.text_cache_index_buffer_sz	 = 1024 * 1024 * 20,
			.buffer_count				 = 5,
		});

		_vekt_data.builder->set_on_draw(on_draw, this);

		_vekt_data.font_manager->init();
		_vekt_data.font_manager->set_callback_user_data(this);
		_vekt_data.font_manager->set_atlas_created_callback(on_atlas_created);
		_vekt_data.font_manager->set_atlas_updated_callback(on_atlas_updated);
		_vekt_data.font_manager->set_atlas_destroyed_callback(on_atlas_destroyed);

#ifdef SFG_TOOLMODE
		const string p		  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		const string p2		  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/icons.ttf");
		_vekt_data.font_debug = _vekt_data.font_manager->load_font_from_file(p.c_str(), DEBUG_FONT_SIZE);
		_vekt_data.font_icon  = _vekt_data.font_manager->load_font_from_file(p2.c_str(), 12, 32, 128, vekt::font_type::sdf);
#else
		SFG_NOTIMPLEMENTED();
#endif

		_vekt_data.console_texts.reserve(MAX_CONSOLE_TEXT);
		_input_field.history.reserve(MAX_CONSOLE_TEXT);

		build_console();

		// load history
		if (file_system::exists(HISTORY_PATH))
		{
			istream in	 = serialization::load_from_file(HISTORY_PATH);
			uint8	size = 0;
			in >> _console_state;
			in >> size;

			set_console_visible(_console_state == console_state::visible);

			for (uint8 i = 0; i < size; i++)
			{
				uint8 len = 0;
				in >> len;

				const char* history = _text_allocator.allocate(static_cast<size_t>(len));
				in.read_to_raw((uint8*)history, static_cast<size_t>(len));
				_input_field.history.push_back(history);
			}

			in.destroy();
		}

		log::instance().add_listener(TO_SIDC("debug_controller"), on_log, this);
	}

	void debug_controller::uninit()
	{

		// save history
		{
			const uint8 history_sz = static_cast<uint8>(_input_field.history.size());
			ostream		out;
			out << _console_state;
			out << history_sz;

			for (const char* el : _input_field.history)
			{
				const uint8 len = static_cast<uint8>(strlen(el));
				out << len;
				out.write_raw((uint8*)el, static_cast<size_t>(len));
			}

			serialization::save_to_file(HISTORY_PATH, out);
			out.destroy();
		}

		_vekt_data.font_manager->unload_font(_vekt_data.font_debug);
		_vekt_data.font_manager->unload_font(_vekt_data.font_icon);
		_vekt_data.font_manager->uninit();
		delete _vekt_data.font_manager;
		_vekt_data.font_manager = nullptr;

		SFG_ASSERT(_gfx_data.atlases.empty());

		_vekt_data.builder->uninit();
		delete _vekt_data.builder;

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_texture(pfd.rt_console);
			backend->destroy_texture(pfd.rt_post);
			backend->unmap_resource(pfd.buf_pass_data.get_gpu());
			pfd.buf_pass_data.destroy();
			pfd.buf_gui_vtx.destroy();
			pfd.buf_gui_idx.destroy();
		}

		log::instance().remove_listener(TO_SIDC("debug_controller"));
		_text_allocator.uninit();
	}

	void debug_controller::prepare(uint8 frame_index)
	{
		ZoneScoped;

		_gfx_data.frame_index = frame_index;

		per_frame_data& pfd = _pfd[frame_index];
		pfd.reset();

		const gui_pass_view view = {
			.proj		   = matrix4x4::ortho_reverse_z(0, static_cast<float>(_gfx_data.rt_size.x), 0, static_cast<float>(_gfx_data.rt_size.y), 0.0f, 1.0f),
			.sdf_thickness = 0.5f,
			.sdf_softness  = 0.02f,
		};
		pfd.buf_pass_data.buffer_data(0, (void*)&view, sizeof(gui_pass_view));

		flush_key_events();

		_vekt_data.builder->build_begin(vector2(_gfx_data.rt_size.x, _gfx_data.rt_size.y));
		console_logic();
		_vekt_data.builder->build_end();
		_vekt_data.builder->flush();
	}

	void debug_controller::tick()
	{
		ZoneScoped;

		const char* cmd = nullptr;
		while (_commands.try_dequeue(cmd))
		{
			debug_console::get()->parse_console_command(cmd);
			SFG_FREE((void*)cmd);
		}
	}

	void debug_controller::render(gfx_id cmd_buffer, uint8 frame_index, bump_allocator& alloc)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		per_frame_data&	  pfd				   = _pfd[frame_index];
		const vector2ui16 rt_size			   = _gfx_data.rt_size;
		const gfx_id	  rt_console		   = pfd.rt_console;
		const gfx_id	  rt_post			   = pfd.rt_post;
		const gfx_id	  gui_vertex		   = pfd.buf_gui_vtx.get_gpu();
		const gfx_id	  gui_index			   = pfd.buf_gui_idx.get_gpu();
		const gfx_id	  shader_fullscreen	   = _shaders.debug_controller_console_draw;
		const uint16	  dc_count			   = pfd.draw_call_count;
		const uint32	  rt_console_gpu_index = pfd.rt_console_index;
		const uint32	  gui_pass_gpu_index   = pfd.buf_pass_data.get_index();

		// Copy vtx idx buffers. First transition barriers will be executed via collect_barriers
		static_vector<barrier, 4> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_render_target,
			.resource	 = pfd.rt_console,
			.flags		 = barrier_flags::baf_is_texture,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_render_target,
			.resource	 = pfd.rt_post,
			.flags		 = barrier_flags::baf_is_texture,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_index_buffer,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.buf_gui_idx.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_vertex_cbv,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.buf_gui_vtx.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		barriers.resize(0);

		if (pfd.counter_vtx != 0)
			pfd.buf_gui_vtx.copy_region(cmd_buffer, 0, pfd.counter_vtx * sizeof(vekt::vertex));

		if (pfd.counter_idx != 0)
			pfd.buf_gui_idx.copy_region(cmd_buffer, 0, pfd.counter_idx * sizeof(vekt::index));

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_index_buffer,
			.resource	 = pfd.buf_gui_idx.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_vertex_cbv,
			.resource	 = pfd.buf_gui_vtx.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		barriers.resize(0);

		render_pass_color_attachment* attachment_console_rt = alloc.allocate<render_pass_color_attachment>(1);
		attachment_console_rt->clear_color					= vector4(0.0f, 0.0f, 0.0f, 0.0f);
		attachment_console_rt->load_op						= load_op::clear;
		attachment_console_rt->store_op						= store_op::store;
		attachment_console_rt->texture						= rt_console;
		attachment_console_rt->view_index					= 0;

		render_pass_color_attachment* attachment_fullscreen_rt = alloc.allocate<render_pass_color_attachment>(1);
		attachment_fullscreen_rt->clear_color				   = vector4(0.0f, 0.0f, 0.0f, 0.0f);
		attachment_fullscreen_rt->load_op					   = load_op::clear;
		attachment_fullscreen_rt->store_op					   = store_op::store;
		attachment_fullscreen_rt->texture					   = rt_post;
		attachment_fullscreen_rt->view_index				   = 0;

		// gui pass bind group
		{
			backend->cmd_bind_constants(cmd_buffer,
										{
											.data		 = (uint8*)&gui_pass_gpu_index,
											.offset		 = constant_index_rp_constant0,
											.count		 = 1,
											.param_index = rpi_constants,
										});
		}

		backend->cmd_set_viewport(cmd_buffer, {.width = static_cast<uint16>(rt_size.x), .height = static_cast<uint16>(rt_size.y)});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "debug_controller_draw");
		backend->cmd_begin_render_pass(cmd_buffer, {.color_attachments = attachment_console_rt, .color_attachment_count = 1});
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
				backend->cmd_bind_constants(cmd_buffer,
											{
												.data		 = (uint8*)&dc.atlas_gpu_index,
												.offset		 = constant_index_object_constant0,
												.count		 = 1,
												.param_index = rpi_constants,
											});
				last_atlas_constant = dc.atlas_gpu_index;
			}

			backend->cmd_draw_indexed_instanced(cmd_buffer, {.index_count_per_instance = dc.index_count, .instance_count = 1, .start_index_location = dc.start_idx, .base_vertex_location = dc.start_vtx, .start_instance_location = 0});
		}

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = rt_console,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		barriers.resize(0);

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "debug_controller_post");
		backend->cmd_begin_render_pass(cmd_buffer, {.color_attachments = attachment_fullscreen_rt, .color_attachment_count = 1});
		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(rt_size.x), .height = static_cast<uint16>(rt_size.y)});
		backend->cmd_set_viewport(cmd_buffer, {.width = static_cast<uint16>(rt_size.x), .height = static_cast<uint16>(rt_size.y)});

		// full-screen bind group
		{
			const uint32 constants[3] = {rt_console_gpu_index, static_cast<uint32>(_gfx_data.rt_size.x), static_cast<uint32>(_gfx_data.rt_size.y)};
			backend->cmd_bind_constants(cmd_buffer,
										{
											.data		 = (uint8*)&constants,
											.offset		 = constant_index_object_constant0,
											.count		 = 3,
											.param_index = rpi_constants,
										});
		}

		// fullscreen draw
		backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = shader_fullscreen});
		backend->cmd_draw_instanced(cmd_buffer, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = rt_post,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		barriers.resize(0);
	}

	void debug_controller::on_draw(const vekt::draw_buffer& buffer, void* ud)
	{
		debug_controller* cont = static_cast<debug_controller*>(ud);

		gfx_backend* backend = gfx_backend::get();

		const vekt::font*	  font			   = buffer.used_font;
		const vekt::atlas*	  atlas			   = font ? font->_atlas : nullptr;
		const vekt::font_type font_type		   = font ? font->type : vekt::font_type::normal;
		const vekt::vertex*	  buffer_vtx_start = buffer.vertex_start;
		const vekt::index*	  buffer_idx_start = buffer.index_start;
		const vector4		  clip			   = buffer.clip;
		const uint32		  buffer_idx_count = buffer.index_count;
		const uint32		  buffer_vtx_count = buffer.vertex_count;
		const gfx_id		  sdf_shader	   = cont->_shaders.gui_sdf;
		const gfx_id		  text_shader	   = cont->_shaders.gui_text;
		const gfx_id		  default_shader   = cont->_shaders.gui_default;

		per_frame_data& pfd			= cont->_pfd[cont->_gfx_data.frame_index];
		const uint32	vtx_counter = pfd.counter_vtx;
		const uint32	idx_counter = pfd.counter_idx;
		const uint32	dc_count	= pfd.draw_call_count;
		pfd.draw_call_count++;
		pfd.counter_vtx += buffer_vtx_count;
		pfd.counter_idx += buffer_idx_count;
		pfd.buf_gui_vtx.buffer_data(sizeof(vekt::vertex) * static_cast<size_t>(vtx_counter), buffer_vtx_start, static_cast<size_t>(buffer_vtx_count) * sizeof(vekt::vertex));
		pfd.buf_gui_idx.buffer_data(sizeof(vekt::index) * static_cast<size_t>(idx_counter), buffer_idx_start, static_cast<size_t>(buffer_idx_count) * sizeof(vekt::index));
		SFG_ASSERT(pfd.draw_call_count < MAX_GUI_DRAW_CALLS);

		gui_draw_call& dc = cont->_gui_draw_calls[dc_count];
		dc				  = {};
		dc.start_idx	  = idx_counter;
		dc.start_vtx	  = vtx_counter;
		dc.index_count	  = static_cast<uint32>(buffer_idx_count);

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
			auto it	  = vector_util::find_if(cont->_gfx_data.atlases, [&](const atlas_ref& ref) -> bool { return ref.atlas == atlas; });
			SFG_ASSERT(it != cont->_gfx_data.atlases.end());
			dc.atlas_gpu_index = it->texture_gpu_index;
		}
		else
		{
			dc.shader = default_shader;
		}
	}

	void debug_controller::on_atlas_created(vekt::atlas* atlas, void* user_data)
	{
		SFG_VERIFY_THREAD_MAIN();
		debug_controller* controller = static_cast<debug_controller*>(user_data);
		gfx_backend*	  backend	 = gfx_backend::get();
		controller->_gfx_data.atlases.push_back({});

		atlas_ref& ref = controller->_gfx_data.atlases.back();
		ref.atlas	   = atlas;
		ref.texture	   = backend->create_texture({
			   .texture_format = atlas->get_is_lcd() ? format::r8g8b8a8_srgb : format::r8_unorm,
			   .size		   = vector2ui16(static_cast<uint16>(atlas->get_width()), static_cast<uint16>(atlas->get_height())),
			   .flags		   = texture_flags::tf_is_2d | texture_flags::tf_sampled,
			   .debug_name	   = "vekt_atlas",
		   });

		const uint32 txt_size	   = backend->get_texture_size(atlas->get_width(), atlas->get_height(), atlas->get_is_lcd() ? 3 : 1);
		const uint32 adjusted_size = backend->align_texture_size(txt_size);
		ref.intermediate_buffer	   = backend->create_resource({
			   .size	   = adjusted_size,
			   .flags	   = resource_flags::rf_cpu_visible,
			   .debug_name = "inter_buffer",
		   });

		ref.texture_gpu_index = backend->get_texture_gpu_index(ref.texture, 0);
	}

	void debug_controller::on_atlas_updated(vekt::atlas* atlas, void* user_data)
	{
		SFG_VERIFY_THREAD_MAIN();
		debug_controller* controller = static_cast<debug_controller*>(user_data);
		gfx_backend*	  backend	 = gfx_backend::get();

		auto it = vector_util::find_if(controller->_gfx_data.atlases, [atlas](const atlas_ref& ref) -> bool { return ref.atlas == atlas; });
		SFG_ASSERT(it != controller->_gfx_data.atlases.end());
		atlas_ref& ref = *it;

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
		controller->_gfx_data.texture_queue->add_request(buffers, ref.texture, ref.intermediate_buffer, 0, resource_state::resource_state_ps_resource);
	}

	void debug_controller::on_atlas_destroyed(vekt::atlas* atlas, void* user_data)
	{
		SFG_VERIFY_THREAD_MAIN();
		debug_controller* controller = static_cast<debug_controller*>(user_data);
		auto			  it		 = vector_util::find_if(controller->_gfx_data.atlases, [atlas](const atlas_ref& ref) -> bool { return ref.atlas == atlas; });
		SFG_ASSERT(it != controller->_gfx_data.atlases.end());
		atlas_ref&	 ref	 = *it;
		gfx_backend* backend = gfx_backend::get();

		backend->destroy_texture(ref.texture);
		backend->destroy_resource(ref.intermediate_buffer);

		controller->_gfx_data.atlases.erase(it);
	}

	void debug_controller::set_console_visible(bool visible)
	{
		_vekt_data.builder->widget_set_visible(_vekt_data.widget_console_bg, visible);
		_vekt_data.builder->widget_set_visible(_vekt_data.widget_input_field, visible);
		_vekt_data.builder->widget_set_visible(_vekt_data.widget_border, visible);
	}

	void debug_controller::console_logic()
	{
		_gfx_data.frame_counter++;
		if (_gfx_data.frame_counter % 120 == 0)
		{
			const vekt::text_props& fps_props	 = _vekt_data.builder->widget_get_text(_vekt_data.widget_fps);
			const vekt::text_props& update_props = _vekt_data.builder->widget_get_text(_vekt_data.widget_main_thread);
			const vekt::text_props& render_props = _vekt_data.builder->widget_get_text(_vekt_data.widget_render_thread);
			string_util::append_float(static_cast<float>(frame_info::get_fps()), (char*)fps_props.text + 5, 4, 1, true);
			string_util::append_float(static_cast<float>(frame_info::get_main_thread_time_milli()), (char*)update_props.text + 6, 7, 4, true);
			string_util::append_float(static_cast<float>(frame_info::get_render_thread_time_milli()), (char*)render_props.text + 8, 7, 4, true);

#ifdef SFG_ENABLE_MEMORY_TRACER
			memory_tracer& tracer = memory_tracer::get();
			LOCK_GUARD(tracer.get_category_mtx());

			const vekt::text_props& glob_mem_props = _vekt_data.builder->widget_get_text(_vekt_data.widget_global_mem);
			const vekt::text_props& gfx_mem_props  = _vekt_data.builder->widget_get_text(_vekt_data.widget_gfx_mem);

			for (const memory_category& cat : tracer.get_categories())
			{
				if (TO_SID(cat.name) == TO_SID("General"))
				{
					string_util::append_float(static_cast<float>(cat.total_size) / B_TO_MB, (char*)glob_mem_props.text + 5, 6, 4, true);
				}
				else if (TO_SID(cat.name) == TO_SID("Gfx"))
				{
					string_util::append_float(static_cast<float>(cat.total_size) / B_TO_MB, (char*)gfx_mem_props.text + 6, 6, 4, true);
				}
			}
#endif
		}

		if (_console_state == console_state::invisible)
			return;

		vekt::pos_props&  console_bg_pos_props	= _vekt_data.builder->widget_get_pos_props(_vekt_data.widget_console_bg);
		vekt::size_props& console_bg_size_props = _vekt_data.builder->widget_get_size_props(_vekt_data.widget_console_bg);
		const vector2&	  console_bg_size		= _vekt_data.builder->widget_get_size(_vekt_data.widget_console_bg);
		const vector2	  pos_text				= _vekt_data.builder->widget_get_pos(_vekt_data.widget_input_text);
		const vector2	  size_text				= _vekt_data.builder->widget_get_size(_vekt_data.widget_input_text);
		const vector2	  pos_field				= _vekt_data.builder->widget_get_pos(_vekt_data.widget_input_field);
		const float		  total_element_size	= _vekt_data.console_total_text_size_y;
		const float		  diff					= total_element_size - (console_bg_size.y - console_bg_size_props.child_margins.top - console_bg_size_props.child_margins.bottom);
		_input_field.scroll_amt					= math::clamp(_input_field.scroll_amt, (int16)0, static_cast<int16>(diff));
		console_bg_pos_props.scroll_offset		= -math::max(diff - _input_field.scroll_amt, 0.0f);

		vekt::widget_gfx gfx = {};

		const float size_per_char = _input_field.text_size == 0 ? 0 : (size_text.x / static_cast<float>(_input_field.text_size));

		const vector2					pos	  = vector2(pos_text.x + (size_per_char * static_cast<float>(_input_field.caret_pos)), pos_field.y + INPUT_FIELD_HEIGHT * 0.25f);
		const vekt::builder::rect_props props = {
			.gfx			 = gfx,
			.min			 = pos,
			.max			 = vector2(pos.x + INPUT_FIELD_HEIGHT * 0.25f, pos.y + INPUT_FIELD_HEIGHT * 0.5f),
			.use_hovered	 = false,
			.use_pressed	 = false,
			.color_start	 = COLOR_TEXT,
			.color_end		 = {},
			.color_direction = vekt::direction::horizontal,
			.widget_id		 = 0,
			.multi_color	 = false,
		};

		_vekt_data.builder->add_filled_rect(props);
	}

	void debug_controller::update_console_input_field()
	{
		_vekt_data.builder->widget_update_text(_vekt_data.widget_input_text);
		_input_field.text_size = strlen(_input_field.text);
		_input_field.caret_pos = math::min(_input_field.caret_pos, _input_field.text_size);
	}

	void debug_controller::on_log(log_level lvl, const char* msg, void* user_data)
	{
		debug_controller* cont = reinterpret_cast<debug_controller*>(user_data);
		cont->add_console_text(msg, lvl);
	}

	void debug_controller::add_console_text(const char* text, log_level level)
	{
		if (level == log_level::trace)
			return;

		_input_field.scroll_amt = 0.0f;

		if (_vekt_data.console_texts.size() == MAX_CONSOLE_TEXT - 1)
		{
			vekt::id		  t	 = _vekt_data.console_texts[0];
			vekt::text_props& tp = _vekt_data.builder->widget_get_text(t);
			_vekt_data.console_total_text_size_y -= _vekt_data.builder->widget_get_size_props(t).size.y + CONSOLE_SPACING;
			_text_allocator.deallocate((char*)tp.text);
			_vekt_data.builder->deallocate(t);
			_vekt_data.console_texts.erase(_vekt_data.console_texts.begin());
		}

		const char* allocated = _text_allocator.allocate(text);
		if (!allocated)
			return;

		vekt::id w = _vekt_data.builder->allocate();
		_vekt_data.builder->widget_set_pos(w, vector2(0.0f, 0.0f));

		vekt::widget_gfx& gfx = _vekt_data.builder->widget_get_gfx(w);
		gfx.flags			  = vekt::gfx_flags::gfx_is_text;
		gfx.color			  = COLOR_TEXT;

		switch (level)
		{
		case log_level::error:
			gfx.color = COLOR_TEXT_ERR;
			break;
		case log_level::warning:
			gfx.color = COLOR_TEXT_WARN;
			break;
		case log_level::progress:
			gfx.color = COLOR_TEXT_PROGRESS;
			break;
		default:
			gfx.color = COLOR_TEXT;
		}

		vekt::text_props& tp = _vekt_data.builder->widget_get_text(w);
		tp.text				 = allocated;
		tp.font				 = _vekt_data.font_debug;
		_vekt_data.builder->widget_update_text(w);
		_vekt_data.builder->widget_add_child(_vekt_data.widget_console_bg, w);

		_vekt_data.console_texts.push_back(w);
		_vekt_data.console_total_text_size_y += _vekt_data.builder->widget_get_size_props(w).size.y + CONSOLE_SPACING;
	}

	void debug_controller::flush_key_events()
	{
		input_event ev = {};

		while (_input_events.try_dequeue(ev))
		{

			if (ev.wheel != 0)
			{
				_input_field.scroll_amt += ev.wheel * 50;
				continue;
			}

			const input_code button = static_cast<input_code>(ev.button);

			if (button == input_code::key_angle_bracket)
			{
				if (_console_state == console_state::visible)
				{
					_console_state = console_state::invisible;
					set_console_visible(false);
					while (_input_events.pop())
						continue;
					return;
				}
				else if (_console_state == console_state::invisible)
				{
					_console_state = console_state::visible;
					set_console_visible(true);

					continue;
				}
			}

			_input_field.text_size = static_cast<int8>(strlen(_input_field.text));
			char* buffer		   = const_cast<char*>(_input_field.text);

			if (button == input_code::key_backspace)
			{
				if (_input_field.text_size != 0)
				{
					for (int i = _input_field.caret_pos; i < _input_field.text_size - 1; i++)
						buffer[i] = buffer[i + 1];

					buffer[_input_field.text_size - 1] = '\0';
					update_console_input_field();
				}

				continue;
			}

			if (button == input_code::key_return)
			{
				if (_input_field.text_size > 0)
				{
					add_console_text(buffer, log_level::info);
					if (_input_field.history.size() >= MAX_HISTORY)
					{
						const char* history = _input_field.history[0];
						_text_allocator.deallocate((char*)history);
						_input_field.history.erase(_input_field.history.begin());
					}

					const char* history_element = _text_allocator.allocate(buffer);
					_input_field.history.push_back(history_element);
					_input_field.history_traversal = static_cast<int8>(_input_field.history.size());

					const size_t buffer_sz = strlen(buffer);
					const char*	 cmd	   = (const char*)SFG_MALLOC(buffer_sz);
					SFG_MEMCPY((char*)cmd, buffer, buffer_sz);
					_commands.emplace(cmd);

					buffer[0] = '\0';
					update_console_input_field();
				}

				continue;
			}

			if (button == input_code::key_up)
			{
				if (_input_field.history.empty())
					continue;

				_input_field.history_traversal = math::max(_input_field.history_traversal - 1, 0);

				const char* history = _input_field.history[_input_field.history_traversal];
				strcpy(buffer, history);
				update_console_input_field();
				_input_field.caret_pos = _input_field.text_size;

				continue;
			}

			if (button == input_code::key_down)
			{
				if (_input_field.history.empty())
					continue;

				_input_field.history_traversal = math::min((int8)(_input_field.history_traversal + 1), static_cast<int8>(_input_field.history.size() - 1));

				const char* history = _input_field.history[_input_field.history_traversal];
				strcpy(buffer, history);
				update_console_input_field();
				_input_field.caret_pos = _input_field.text_size;

				continue;
			}

			if (button == input_code::key_left)
			{
				_input_field.caret_pos = math::max(0, _input_field.caret_pos - 1);
				continue;
			}

			if (button == input_code::key_right)
			{
				_input_field.caret_pos = math::min(static_cast<int8>(_input_field.text_size), static_cast<int8>(_input_field.caret_pos + 1));
				continue;
			}

			if (_input_field.text_size >= MAX_INPUT_FIELD)
				continue;

			const char	 c	  = process::get_character_from_key(static_cast<uint32>(ev.button));
			const uint16 mask = process::get_character_mask_from_key(static_cast<uint32>(ev.button), c);

			if (!(mask & character_mask::printable))
				continue;

			for (int i = _input_field.text_size; i > _input_field.caret_pos; --i)
			{
				buffer[i] = buffer[i - 1];
			}

			buffer[_input_field.caret_pos]	   = c;
			buffer[_input_field.text_size + 1] = '\0';

			_input_field.caret_pos++;
			_input_field.text_size++;
			update_console_input_field();
		}
	}

	bool debug_controller::on_window_event(const window_event& ev)
	{
		const bool def_val = _console_state == console_state::visible;

		if (ev.type == window_event_type::key && ev.sub_type != window_event_sub_type::release)
		{
			if (_console_state == console_state::invisible && static_cast<input_code>(ev.button) != input_code::key_angle_bracket)
				return def_val;

			const input_event ke = {.button = static_cast<uint16>(ev.button)};
			_input_events.try_enqueue(ke);
			return true;
		}

		if (ev.type == window_event_type::wheel)
		{
			const input_event ke = {.wheel = ev.value.y};
			_input_events.try_enqueue(ke);
			return true;
		}

		return def_val;
	}

	void debug_controller::on_window_resize(const vector2ui16& size)
	{
		gfx_backend* backend  = gfx_backend::get();
		_gfx_data.rt_size	  = vector2ui16(size.x, size.y / 2);
		_gfx_data.screen_size = vector2ui16(size.x, size.y);

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_texture(pfd.rt_console);
			backend->destroy_texture(pfd.rt_post);

			pfd.rt_console = backend->create_texture({
				.texture_format = RT_FORMAT,
				.size			= vector2ui16(size.x, size.y / 2),
				.flags			= texture_flags::tf_sampled | texture_flags::tf_is_2d | texture_flags::tf_render_target,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "console_rt",
			});

			pfd.rt_post = backend->create_texture({
				.texture_format = RT_FORMAT,
				.size			= vector2ui16(size.x, size.y / 2),
				.flags			= texture_flags::tf_sampled | texture_flags::tf_is_2d | texture_flags::tf_render_target,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "debug_rt",
			});

			pfd.rt_console_index	= backend->get_texture_gpu_index(pfd.rt_console, 1);
			pfd.rt_fullscreen_index = backend->get_texture_gpu_index(pfd.rt_post, 1);
		}
	}
}