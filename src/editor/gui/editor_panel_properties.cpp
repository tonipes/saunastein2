#include "editor_panel_properties.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "gui/vekt.hpp"
#include "imgui.h"

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

	void editor_panel_properties::init(vekt::builder* builder)
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

	void editor_panel_properties::uninit()
	{
		_builder->deallocate(_w_window);
		_w_window = NULL_WIDGET_ID;
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
					ImGui::Text("name: %s", meta.name);
					ImGui::Text("children: %u", get_direct_child_count(em, selected));
					ImGui::Text("index: %u", selected.index);
					ImGui::Text("generation: %u", selected.generation);

					ImGui::SeparatorText("transform");
					vector3	   pos	 = em.get_entity_position(selected);
					vector3	   scale = em.get_entity_scale(selected);
					const quat rotq	 = em.get_entity_rotation(selected);
					vector3	   euler = quat::to_euler(rotq);

					float p[3] = {pos.x, pos.y, pos.z};

					if (ImGui::DragFloat3("position", p, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
					{
						em.set_entity_position(selected, vector3(p[0], p[1], p[2]));
					}

					float r[3] = {euler.x, euler.y, euler.z};
					if (ImGui::DragFloat3("rotation", r, 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
					{
						em.set_entity_rotation(selected, quat::from_euler(r[0], r[1], r[2]));
					}

					float s[3] = {scale.x, scale.y, scale.z};
					if (ImGui::DragFloat3("scale", s, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ColorMarkers))
					{
						em.set_entity_scale(selected, vector3(s[0], s[1], s[2]));
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
