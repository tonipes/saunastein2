#include "editor_panel_entities.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "gui/vekt.hpp"
#include "imgui.h"
#include <vector>

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
			if (ImGui::MenuItem("Add Child Entity"))
			{
				world_handle child = em.create_entity("entity");
				em.add_child(e, child);
			}
			if (ImGui::MenuItem("Delete Entity"))
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

	void editor_panel_entities::init(vekt::builder* builder)
	{
		_builder			  = builder;
		_w_window			  = builder->allocate();
		vekt::widget_gfx& gfx = builder->widget_get_gfx(_w_window);
		gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
		gfx.color			  = vector4(0.1f, 0.1f, 0.1f, 1.0f);
		_builder->widget_add_child(_builder->get_root(), _w_window);
		builder->widget_set_size_abs(_w_window, vector2(200, 300));
		builder->widget_set_pos_abs(_w_window, vector2(200, 300));
	}

	void editor_panel_entities::uninit()
	{
		_builder->deallocate(_w_window);
		_w_window = NULL_WIDGET_ID;
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
			entity_manager&			  em	   = w.get_entity_manager();
			auto&					  entities = *em.get_entities();
			std::vector<world_handle> roots;
			roots.reserve(256);
			for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
			{
				world_handle		 e	 = *it;
				const entity_family& fam = em.get_entity_family(e);
				if (fam.parent.is_null())
					roots.push_back(e);
			}
			for (world_handle r : roots)
				draw_entity_node(em, r);
			if (ImGui::BeginPopupContextWindow("EntitiesChildCtx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Add Root Entity"))
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
