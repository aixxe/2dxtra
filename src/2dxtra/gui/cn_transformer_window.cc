#include <iomanip>
#include <sstream>
#include <imgui.h>
#include "../input.h"
#include "../features/cn_override.h"
#include "../features/cn_transformer.h"
#include "cn_transformer_window.h"

namespace iidxtra::gui::cn_transformer_window
{
	auto visible = false;

    auto draw_player_config_tab(std::uint8_t player) -> void
    {
        if (!ImGui::BeginTabItem((player == 0 ? "P1/LEFT": "P2/RIGHT")))
            return;

        // player-specific data
        auto& transformer_enabled = (player == 0 ? cn_transformer::enabled_p1 : cn_transformer::enabled_p2);
        auto& long_note_percentage = (player == 0 ? cn_transformer::long_note_percentage_p1: cn_transformer::long_note_percentage_p2);
        auto& allow_backspin_scratch = (player == 0 ? cn_transformer::allow_backspin_scratch_p1: cn_transformer::allow_backspin_scratch_p2);
        auto& override_type = (player == 0 ? cn_override::p1_override_type: cn_override::p2_override_type);
        auto& max_cn_duration = (player == 0 ? cn_transformer::max_cn_duration_p1: cn_transformer::max_cn_duration_p2);

        // Charge Note Type
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                const char* names[] = {"Use Chart Default", "Charge Note", "Hell Charge Note"};

                ImGui::Text("Charge Note Type");
                ImGui::SameLine();
                ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(150);
                if (ImGui::SliderInt("##CNType", &override_type, 0, 2, names[override_type]))
                    cn_override::update(player);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Forces charts to use either CNs or HCNs");
        }

        ImGui::Separator();

        // Enable CN Transformer
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Enable CN Transformer");
                ImGui::SameLine();
                ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                ImGui::SameLine(300);
                ImGui::Checkbox("##EnableCNTransformer", &transformer_enabled);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Merges regular notes to create charge notes");
        }

        // Note Intensity
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Note Intensity");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(150);
                int note_percentage = (long_note_percentage / 5);
                if (ImGui::SliderInt("##NoteIntensity", &note_percentage, 1, 20))
                    long_note_percentage = (note_percentage * 5);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Rate for CNs to be generated");
        }

        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Max Note Duration");
                ImGui::SameLine(300);
                ImGui::SetNextItemWidth(150);

                auto max_note_duration = (max_cn_duration / 100);
                auto display = std::stringstream {};
                if (max_note_duration == 0)
                    display << "No limit";
                else
                    display << std::fixed << std::setprecision(2) << (float(max_note_duration) / 10.f) << " seconds";

                if (ImGui::SliderInt("##MaxNoteDuration", &max_note_duration, 0, 100, display.str().c_str()))
                {
                    auto real_duration = (max_note_duration * 100);

                    if (real_duration == 0)
                        max_cn_duration = -1;
                    else
                        max_cn_duration = real_duration;
                }
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Length time limit for each charge note");
        }

        // Enable Scratch Lane
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Enable Scratch Lane");
                ImGui::SameLine(300);
                ImGui::Checkbox("##AllowScratchCN", &allow_backspin_scratch);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Create backspin scratches");
        }

        // back button
        if (ImGui::Button("Return to main menu"))
            visible = false;

        ImGui::EndTabItem();
    }

	void render()
	{
		ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f}, 0, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({485, 240});

		ImGui::Begin("Configuration :: CN Transformer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        if (bm2dx::state->play_style == 0 && !bm2dx::state->p1_active && !bm2dx::state->p2_active)
        {
            ImGui::Text("Options will appear here once a player is active");

            if (ImGui::Button("Return to main menu"))
                visible = false;

            return ImGui::End();
        }

		ImGui::BeginTabBar("CN Transformer");

        if (bm2dx::state->play_style == 0)
        {
            if (bm2dx::state->p1_active)
                draw_player_config_tab(0);
            if (bm2dx::state->p2_active)
                draw_player_config_tab(1);
        }
        else
        {
            draw_player_config_tab(0);
            draw_player_config_tab(1);
        }

        ImGui::EndTabBar();
		ImGui::End();
	}
}