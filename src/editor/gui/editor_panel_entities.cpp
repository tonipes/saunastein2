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

#include "editor_panel_entities.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "gui/vekt.hpp"
#include "imgui.h"

namespace SFG
{
	static void draw_entity_node(entity_manager& em, world_handle e)
	{
		const entity_meta& meta	 = em.get_entity_meta(e);
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		bool			   open	 = ImGui::TreeNodeEx((void*)(uintptr_t)e.index, flags, "%s", meta.name);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			editor::get().set_selected_entity(e);
		}
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("add child"))
			{
				world_handle child = em.create_entity("entity");
				em.add_child(e, child);
			}
			if (ImGui::MenuItem("delete"))
			{
				em.destroy_entity(e);
				ImGui::EndPopup();
				if (open)
					ImGui::TreePop();
				return;
			}
			ImGui::EndPopup();
		}

		if (open)
		{
			const entity_family& fam = em.get_entity_family(e);
			world_handle		 c	 = fam.first_child;
			while (!c.is_null())
			{
				world_handle next = em.get_entity_family(c).next_sibling;
				draw_entity_node(em, c);
				c = next;
			}
			ImGui::TreePop();
		}
	}

	void editor_panel_entities::init()
	{
		
	}

	void editor_panel_entities::uninit()
	{
	}

	void editor_panel_entities::draw(world& w, const vector2ui16& window_size)
	{
		const ImVec2 initial_pos(0.0f, 0.0f);
		const ImVec2 initial_size(static_cast<float>(window_size.x) * 0.25f, static_cast<float>(window_size.y) * 0.6f);
		ImGui::SetNextWindowPos(initial_pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(initial_size, ImGuiCond_FirstUseEver);
		static bool open = true;
		if (ImGui::Begin("Entities", &open))
		{
			ImGui::BeginChild("EntitiesChild", ImVec2(0.0f, 0.0f), false);
			entity_manager& em		 = w.get_entity_manager();
			auto&			entities = *em.get_entities();
			_reuse_roots.resize(0);
			for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
			{
				world_handle		 e	 = *it;
				const entity_family& fam = em.get_entity_family(e);
				if (fam.parent.is_null())
					_reuse_roots.push_back(e);
			}
			for (const world_handle r : _reuse_roots)
				draw_entity_node(em, r);
			if (ImGui::BeginPopupContextWindow("EntitiesChildCtx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("add root"))
				{
					w.get_entity_manager().create_entity("entity");
				}
				ImGui::EndPopup();
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}
}
