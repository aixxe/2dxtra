#include <meta.h>
#include "gui.h"
#include "../input.h"
#include "../features/unrandomizer.h"
#include "../features/scratch_swap.h"
#include "unrandomizer_window.h"

namespace iidxtra::gui::unrandomizer_window
{
	auto visible = false;

	auto in_bind_mode_p1 = false;
	auto in_bind_mode_p2 = false;

	auto in_bind_key_p1 = std::uint8_t { 0 };
	auto in_bind_key_p2 = std::uint8_t { 0 };

    auto draw_player_config_tab(const std::uint8_t player) -> void
    {
        if (!ImGui::BeginTabItem((player == 0 ? "P1/LEFT": "P2/RIGHT")))
            return;

        // player-specific data
        auto& force_random_enabled = (player == 0 ? unrandomizer::enabled_p1: unrandomizer::enabled_p2);
        auto& allow_score_saving = (player == 0 ? unrandomizer::allow_score_save_p1: unrandomizer::allow_score_save_p2);
        auto& scratch_swap_enabled = (player == 0 ? scratch_swap::enabled_p1: scratch_swap::enabled_p2);
        auto& column_lut = (player == 0 ? unrandomizer::column_lut_p1 : unrandomizer::column_lut_p2);
        auto& swap_column = (player == 0 ? scratch_swap::swap_lane_p1: scratch_swap::swap_lane_p2);

        auto& in_bind_mode = (player == 0 ? in_bind_mode_p1: in_bind_mode_p2);
        auto& in_bind_key = (player == 0 ? in_bind_key_p1: in_bind_key_p2);
        auto& show_random_info = (player == 0 ? unrandomizer::show_random_info_p1: unrandomizer::show_random_info_p2);

        // Use Force Random
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Enable Force Random");
                if (!allow_score_saving)
                {
                    ImGui::SameLine();
                    ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                }
                ImGui::SameLine(350);
                ImGui::Checkbox("##UseForceRandom", &force_random_enabled);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Use settings below. Requires RANDOM to be enabled");
        }

        // Save Force Random Scores
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Save Force Random Scores");
                ImGui::SameLine(350);
                #if BLOCK_FORCE_RANDOM_SAVE == 1
                allow_score_saving = false;
                ImGui::BeginDisabled();
                ImGui::Checkbox("##AllowForceRandomSave", &allow_score_saving);
                ImGui::EndDisabled();
                #else
                ImGui::Checkbox("##AllowForceRandomSave", &allow_score_saving);
                #endif
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Allow force random scores to be saved to network");
            ImGui::Separator();
        }

        // Force Random
        ImGui::BeginDisabled(!force_random_enabled);
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Force Random");
                ImGui::SameLine(250);

                // print currently bound keys
                for (auto i = 0; i < 7; i++)
                {
                    if (in_bind_mode && in_bind_key == i)
                        ImGui::Text("%i ", column_lut[i] + 1);
                    else if (in_bind_mode && i < in_bind_key)
                        ImGui::TextColored(ImVec4(0, 255, 0, 255), "%i ", column_lut[i] + 1);
                    else
                        ImGui::TextDisabled("%i ", column_lut[i] + 1);

                    if (i < 6)
                        ImGui::SameLine();
                }

                if (in_bind_mode)
                {
                    // check for key presses
                    for (auto i = 0; i < 7; i++)
                    {
                        auto const button = (player == 0 ? i: i + 7);

                        if (!input::test_menu_button(button))
                            continue;

                        bool allowed = true;

                        for (auto j = 0; j < in_bind_key; j++)
                        {
                            if (column_lut[j] == i)
                            {
                                allowed = false;
                                break;
                            }
                        }

                        if (!allowed)
                            continue;

                        column_lut[in_bind_key] = i;

                        if (in_bind_key == 6)
                        {
                            in_bind_mode = false;
                            input_enabled = true;
                            break;
                        }

                        in_bind_key++;

                        break;
                    }
                }

                ImGui::SameLine(350);

                if (in_bind_mode)
                    ImGui::BeginDisabled(true);

                if (ImGui::Button("Input"))
                {
                    in_bind_mode = true;
                    in_bind_key = 0;
                    input_enabled = false;
                    ImGui::BeginDisabled(true);
                }
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                if (ImGui::Button("Reset"))
                {
                    column_lut[0] = 0;
                    column_lut[1] = 1;
                    column_lut[2] = 2;
                    column_lut[3] = 3;
                    column_lut[4] = 4;
                    column_lut[5] = 5;
                    column_lut[6] = 6;
                }

                if (in_bind_mode)
                    ImGui::EndDisabled();
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Use a pre-determined note column order");

            if (in_bind_mode)
            {
                ImGui::SameLine(250);
                ImGui::Text("> SET COLUMN %i", in_bind_key + 1);
            }
        }

        // Column Shift
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Column Shift");
                ImGui::SameLine(350);

                // 7123456 -> 1234567
                if (ImGui::Button("<<"))
                {
                    auto prev = column_lut[0];
                    column_lut[0] = column_lut[1];
                    column_lut[1] = column_lut[2];
                    column_lut[2] = column_lut[3];
                    column_lut[3] = column_lut[4];
                    column_lut[4] = column_lut[5];
                    column_lut[5] = column_lut[6];
                    column_lut[6] = prev;
                }
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                // 1234567 -> 7123456
                if (ImGui::Button(">>"))
                {
                    auto prev = column_lut[6];
                    column_lut[6] = column_lut[5];
                    column_lut[5] = column_lut[4];
                    column_lut[4] = column_lut[3];
                    column_lut[3] = column_lut[2];
                    column_lut[2] = column_lut[1];
                    column_lut[1] = column_lut[0];
                    column_lut[0] = prev;
                }
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Move all notes one column to the left or right");
            ImGui::Separator();
        }
        ImGui::EndDisabled();

        // Swap Scratch Lane
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Scratch Swap");
                ImGui::SameLine();
                ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                ImGui::SameLine(350);
                ImGui::Checkbox("##ScratchSwapEnabled", &scratch_swap_enabled);
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                if (ImGui::ArrowButton("##ScratchSwapLeft", ImGuiDir_Left))
                    swap_column = (swap_column - 1 < -1 ? 6: swap_column - 1);
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                if (ImGui::ArrowButton("##ScratchSwapRight", ImGuiDir_Right))
                    swap_column = (swap_column + 1 > 6 ? -1: swap_column + 1);
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                if (swap_column == -1)
                    ImGui::Text("Random");
                else
                    ImGui::Text("%i", swap_column + 1);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Switches a note lane with the scratch lane");
            ImGui::Separator();
        }

        // Show Random Info
        {
            auto show_random_info_checkbox = (show_random_info || force_random_enabled);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Display Random Info");
                ImGui::SameLine(350);
                ImGui::BeginDisabled(force_random_enabled);
                if (ImGui::Checkbox("##ShowRandomInfo", &show_random_info_checkbox))
                    show_random_info = show_random_info_checkbox;
                ImGui::EndDisabled();
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Show note arrangement before chart begins");
        }

        // back button
        if (ImGui::Button("Return to main menu"))
            visible = false;

        ImGui::EndTabItem();
    }

	auto render() -> void
    {
		ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f}, 0, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({485, 275});

		ImGui::Begin("Configuration :: Un-randomizer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        if (bm2dx::state->play_style == 0 && !bm2dx::state->p1_active && !bm2dx::state->p2_active)
        {
            // exit bind mode if we were stuck in it
            if (in_bind_mode_p1 || in_bind_mode_p2)
            {
                input_enabled = true;

                in_bind_mode_p1 = false;
                in_bind_mode_p2 = false;
            }

            ImGui::Text("Options will appear here once a player is active");

            if (ImGui::Button("Return to main menu"))
                visible = false;

            return ImGui::End();
        }

		ImGui::BeginTabBar("Un-randomizer");

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