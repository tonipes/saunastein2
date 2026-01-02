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
#include "editor_panels_docking.hpp"
#include "math/vector2ui16.hpp"
#include "imgui.h"
#include "imgui_internal.h"

namespace SFG
{
	void editor_panels_docking::init()
	{
	}

	void editor_panels_docking::uninit()
	{
	}

	void editor_panels_docking::draw(const vector2ui16& window_size)
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + 40 ));
		ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, vp->WorkSize.y - 40));
		ImGui::SetNextWindowViewport(vp->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		bool open = true;
		if (ImGui::Begin("DockSpace", &open, flags))
		{
			ImGuiID dockspace_id = ImGui::GetID("StakeforgeEditorDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
			if (!_built)
			{
				ImGui::DockBuilderRemoveNode(dockspace_id);
				ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);
				ImGuiID dock_main_id		= dockspace_id;
				ImGuiID dock_id_left		= ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
				ImGuiID dock_id_left_bottom = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.2f, nullptr, &dock_id_left);
				ImGuiID dock_id_right		= ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
				ImGuiID dock_id_up			= ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.2f, nullptr, &dock_main_id);
				ImGui::DockBuilderDockWindow("Entities", dock_id_left);
				ImGui::DockBuilderDockWindow("Properties", dock_id_right);
				ImGui::DockBuilderDockWindow("Controls", dock_id_left_bottom);
				ImGui::DockBuilderDockWindow("World View", dock_main_id);
				ImGui::DockBuilderFinish(dockspace_id);
				_built = true;
			}
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::End();
	}
}
