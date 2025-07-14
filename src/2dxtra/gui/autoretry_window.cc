#include <imgui.h>
#include "../features/autoretry.h"
#include "autoretry_window.h"

namespace iidxtra::gui::autoretry_window
{
	auto visible = false;

    auto render() -> void
    {
		ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f}, 0, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({485, 275});

		ImGui::Begin("Configuration :: Auto Retry", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        // Enable Auto Retry
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Enable Auto Retry");
                ImGui::SameLine(350);
                ImGui::Checkbox("##EnableAutoRetry", &autoretry::enabled);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Retry when unable to reach pacemaker score");
            ImGui::Separator();
        }

        // Target Display Type
        {
            ImGui::Text("Target Display Type");
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Replaces the TARGET:+/- or ghost text");

            ImGui::RadioButton("Off", (int*) &autoretry::target, (int) autoretry::target_mode::Off);
            ImGui::SameLine();
            ImGui::TextColored({0.65f, 0.65f, 0.65f, 1.f}, "- Use default game behaviour");

            ImGui::RadioButton("Delta", (int*) &autoretry::target, (int) autoretry::target_mode::Delta);
            ImGui::SameLine();
            ImGui::TextColored({0.65f, 0.65f, 0.65f, 1.f}, "- Show remaining EX score until failure");

            ImGui::RadioButton("Best", (int*) &autoretry::target, (int) autoretry::target_mode::Maximum);
            ImGui::SameLine();
            ImGui::TextColored({0.65f, 0.65f, 0.65f, 1.f}, "- Show best possible EX score");
        }

        // Target Text Destination
        {
            ImGui::Text("Target Text Destination");
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Determine where to display the above altered text");

            ImGui::RadioButton("Graph", (int*) &autoretry::destination, (int) autoretry::target_destination::Graph);
            ImGui::SameLine();
            ImGui::TextColored({0.65f, 0.65f, 0.65f, 1.f}, "- Target text in the corner of the score graph");

            ImGui::RadioButton("Ghost", (int*) &autoretry::destination, (int) autoretry::target_destination::Ghost);
            ImGui::SameLine();
            ImGui::TextColored({0.65f, 0.65f, 0.65f, 1.f}, "- Ghost text in or next to the lane");
        }

        // back button
        if (ImGui::Button("Return to main menu"))
            visible = false;

		ImGui::End();
	}
}