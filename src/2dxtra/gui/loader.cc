#include <imgui.h>
#include "../chart_set.h"
#include "loader.h"

namespace iidxtra::gui
{
    auto render_loader() -> void
    {
		auto const flags = chart_set::switch_enabled ? ImGuiSelectableFlags_None: ImGuiSelectableFlags_Disabled;

		if (!ImGui::CollapsingHeader("Loader", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		ImGui::Text("Available chart sets:");
		ImGui::Indent(10);
			if (ImGui::Selectable("Default", chart_set::active.empty(), flags))
				chart_set::revert();

			for (auto const& [name, set]: chart_set::custom)
			{
				if (ImGui::Selectable(name.c_str(), chart_set::active == name, flags))
					chart_set::set_active(name);

				ImGui::SameLine(300); ImGui::Text("%llu charts", set.count);
			}
		ImGui::Unindent(10);

		if (!chart_set::switch_enabled)
			ImGui::TextColored({0.5f, 0.2f, 0.2f, 1.f}, "The active chart set can only be changed in music select");
    }
}