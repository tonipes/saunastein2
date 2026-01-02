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

#include "editor_panel_properties.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "gui/vekt.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace SFG
{
	static uint32 get_direct_child_count(entity_manager& em, world_handle e)
	{
		const entity_family& fam = em.get_entity_family(e);
		uint32				 c	 = 0;
		world_handle		 ch	 = fam.first_child;
		while (!ch.is_null())
		{
			c++;
			ch = em.get_entity_family(ch).next_sibling;
		}
		return c;
	}

	void editor_panel_properties::init()
	{
	}

	void editor_panel_properties::uninit()
	{
	}

	void editor_panel_properties::draw(world& w, world_handle selected, const vector2ui16& window_size)
	{
		const ImVec2 initial_pos(static_cast<float>(window_size.x) * 0.75f, 0.0f);
		const ImVec2 initial_size(static_cast<float>(window_size.x) * 0.25f, static_cast<float>(window_size.y) * 0.6f);
		ImGui::SetNextWindowPos(initial_pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(initial_size, ImGuiCond_FirstUseEver);
		static bool open = true;
		if (ImGui::Begin("Properties", &open))
		{
			ImGui::BeginChild("PropertiesChild", ImVec2(0.0f, 0.0f), false);
			ImGui::Indent();

			if (!selected.is_null() && _type == editor_property_type::entity)
			{
				entity_manager& em = w.get_entity_manager();
				if (em.is_valid(selected))
				{
					const entity_meta& meta = em.get_entity_meta(selected);
					ImGui::SeparatorText("general");
					static world_handle prev_sel	  = {};
					static char			name_buf[128] = {};
					if (prev_sel.index != selected.index || prev_sel.generation != selected.generation)
					{
						std::strncpy(name_buf, meta.name ? meta.name : "", sizeof(name_buf) - 1);
						name_buf[sizeof(name_buf) - 1] = '\0';
						prev_sel					   = selected;
					}

					if (ImGui::BeginTable("props_general", 2, ImGuiTableFlags_BordersInnerV))
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("name");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						if (ImGui::InputText("##name", name_buf, IM_ARRAYSIZE(name_buf)))
							em.set_entity_name(selected, name_buf);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("children");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						ImGui::Text("%u", get_direct_child_count(em, selected));

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("handle");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						ImGui::Text("{ %u, %u }", selected.index, selected.generation);

						ImGui::EndTable();
					}

					ImGui::SeparatorText("transform");
					vector3	   pos	 = em.get_entity_position(selected);
					vector3	   scale = em.get_entity_scale(selected);
					const quat rotq	 = em.get_entity_rotation(selected);
					vector3	   euler = quat::to_euler(rotq);

					if (ImGui::BeginTable("props_transform", 2, ImGuiTableFlags_BordersInnerV))
					{
						float p[3] = {pos.x, pos.y, pos.z};
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("position");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						if (ImGui::DragFloat3("##position", p, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
							em.set_entity_position(selected, vector3(p[0], p[1], p[2]));

						float r[3] = {euler.x, euler.y, euler.z};
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("rotation");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						if (ImGui::DragFloat3("##rotation", r, 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
							em.set_entity_rotation(selected, quat::from_euler(r[0], r[1], r[2]));

						float s[3] = {scale.x, scale.y, scale.z};
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("scale");
						ImGui::TableSetColumnIndex(1);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
						if (ImGui::DragFloat3("##scale", s, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
							em.set_entity_scale(selected, vector3(s[0], s[1], s[2]));

						ImGui::EndTable();
					}

					ImGui::SeparatorText("components");
				}
			}
			ImGui::Unindent();
			ImGui::EndChild();
		}
		ImGui::End();
	}
}
