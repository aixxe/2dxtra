#include <imgui.h>
#include <algorithm>
#include "../log.h"
#include "log_window.h"

namespace iidxtra::gui::log_window
{
	auto render() -> void
	{
		if (!log::empty())
		{
			auto const lines = log::lines.size();
			auto const opacity = log::opacity();
			auto const height = (ImGui::GetTextLineHeightWithSpacing() * lines) + 15;

			ImGui::SetNextWindowPos({10, (ImGui::GetIO().DisplaySize.y - height - 10) }, 0);
			ImGui::SetNextWindowSize({450,  height});
			ImGui::SetNextWindowBgAlpha(std::clamp(opacity, 0.f, 0.85f));

			ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoDecoration);

			auto it = std::begin(log::lines);

			while (it != std::end(log::lines))
			{
				it->first--;

				if (it->first <= 0)
				{
					it = log::lines.erase(it);
				}
				else
				{
					ImGui::TextColored({1.f, 1.f, 1.f, opacity}, "%s", it->second.c_str());
					++it;
				}
			}

			ImGui::End();
		}
	}
}