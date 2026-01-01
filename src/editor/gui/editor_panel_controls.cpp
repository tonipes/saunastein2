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

#include "editor_panel_controls.hpp"
#include "math/vector2ui16.hpp"
#include "common/system_info.hpp"
#include "common/string_id.hpp"
#include "memory/memory_tracer.hpp"

#include "editor/editor.hpp"
#include "gui/vekt.hpp"

namespace SFG
{
	void editor_panel_controls::init(vekt::builder* builder)
	{
		_builder  = builder;
		_w_window = builder->allocate();

		vekt::widget_gfx& gfx = builder->widget_get_gfx(_w_window);
		gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
		gfx.color			  = vector4(0.1f, 0.1f, 0.1f, 1.0f);

		_builder->widget_add_child(_builder->get_root(), _w_window);

		builder->widget_set_size_abs(_w_window, vector2(200, 300));
		builder->widget_set_pos_abs(_w_window, vector2(200, 300));
	}

	void editor_panel_controls::uninit()
	{
		_builder->deallocate(_w_window);
		_w_window = NULL_WIDGET_ID;
	}

	void editor_panel_controls::draw(const vector2ui16& window_size)
	{

		/*
		const ImVec2 initial_pos(0.0f, 0.0f);
		const ImVec2 initial_size(static_cast<float>(window_size.x) * 0.25f, static_cast<float>(window_size.y) * 0.15f);

		ImGui::SetNextWindowPos(initial_pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(initial_size, ImGuiCond_FirstUseEver);

		static bool open = true;
		if (ImGui::Begin("Controls", &open, ImGuiWindowFlags_MenuBar))
		{
			// Menu bar with a single File menu
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					ImGui::MenuItem("New Level");
					ImGui::MenuItem("Load Level");
					ImGui::MenuItem("Save Level");
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			// Content area: child taking full remaining space
			ImGui::BeginChild("ControlsChild", ImVec2(0.0f, 0.0f), false);
			ImGui::Indent();

			ImGui::SeparatorText("About");

			ImGui::Text("Stakeforge v.%d.%d.%s", SFG_MAJOR, SFG_MINOR, SFG_BUILD);
			ImGui::TextLinkOpenURL("Stakeforge Github", "https://github.com/inanevin/stakeforge");

			ImGui::SeparatorText("Stats");

			static uint32 fps		= frame_info::get_fps();
			static float  main_ms	= frame_info::get_main_thread_time_milli();
			static float  render_ms = frame_info::get_render_thread_time_milli();

			ImGui::Text("fps: %d", fps);
			ImGui::Text("main: %f ms", main_ms);
			ImGui::Text("render: %f ms", render_ms);

			static uint32 ctr = 0;
			if (ctr > 60)
			{
				ctr		  = 0;
				fps		  = frame_info::get_fps();
				main_ms	  = frame_info::get_main_thread_time_milli();
				render_ms = frame_info::get_render_thread_time_milli();
			}
			ctr++;

#ifdef SFG_ENABLE_MEMORY_TRACER
			memory_tracer& tracer = memory_tracer::get();
			LOCK_GUARD(tracer.get_category_mtx());

			for (const memory_category& cat : tracer.get_categories())
			{
				if (TO_SID(cat.name) == TO_SID("General"))
				{
					const uint32 d = static_cast<uint32>(static_cast<double>(cat.total_size) / (1024.0 * 1024.0));
					ImGui::Text("ram: %d mb", d);
				}
				else if (TO_SID(cat.name) == TO_SID("Gfx"))
				{
					const uint32 d = static_cast<uint32>(static_cast<double>(cat.total_size) / (1024.0 * 1024.0));
					ImGui::Text("vram: %d mb", d);
				}
			}
#endif

			ImGui::Unindent();

			ImGui::EndChild();
		}
		ImGui::End();*/
	}
}
