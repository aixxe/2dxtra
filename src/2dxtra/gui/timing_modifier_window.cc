#include "gui.h"
#include "../input.h"
#include "../features/timing_modifier.h"
#include "timing_modifier_window.h"

namespace iidxtra::gui::timing_modifier_window
{
	auto visible = false;

    auto constexpr increment = (1.f * 1000.f / 60);

    auto inline timing_to_color(const bm2dx::timing_t& window, float i) -> ImU32
    {
        for (auto type = 0; type < 9; ++type)
        {
            auto frames = 0.f;
            auto result = 0U;

            switch (type)
            {
                case 0: frames = window.early_poor;   result = IM_COL32(255, 0,   0,   255); break;
                case 1: frames = window.early_bad;    result = IM_COL32(255, 128, 0,   255); break;
                case 2: frames = window.early_good;   result = IM_COL32(255, 255, 0,   255); break;
                case 3: frames = window.early_great;  result = IM_COL32(0,   255, 255, 255); break;
                case 4: frames = window.early_pgreat; result = IM_COL32(255, 255, 255, 255); break;
                case 5: frames = window.late_pgreat;  result = IM_COL32(255, 255, 255, 255); break;
                case 6: frames = window.late_great;   result = IM_COL32(0,   255, 255, 255); break;
                case 7: frames = window.late_good;    result = IM_COL32(255, 255, 0,   255); break;
                case 8: frames = window.late_bad;     result = IM_COL32(255, 128, 0,   255); break;
                default:                                                                     break;
            }

            if (frames >= i) // if ( *(float *)(v37 + 4i64 * i) >= a4 )
                return result;
        }

        return IM_COL32(0, 0, 0, 0);
    }

    auto inline draw_timing_visualizer(const bm2dx::timing_t& window) -> void
    {
        auto const begin = std::min({ window.early_poor, window.early_bad, window.early_good, window.early_great, window.early_pgreat });
        auto const end = std::max({ window.late_bad, window.late_good, window.late_great, window.late_pgreat });

        ImGui::Begin("TimingVisualizer", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize |
                                                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

        auto const size = ImGui::GetWindowSize();
        auto center = ImGui::GetIO().DisplaySize;
        center.x *= 0.5f;
        center.y *= 0.5f;
        center.x -= size.x * 0.5f;
        center.y -= size.y * 0.5f;
        center.y += 180;
        ImGui::SetWindowPos(center);

        auto i = 0.5f;
        auto blocks = std::vector<std::pair<ImU32, float>> {};

        while (i > begin)
            i -= increment;

        while (i < end)
        {
            i += increment;
            auto const color = timing_to_color(window, i);
            blocks.emplace_back(color, i);
        }

        for (auto const& [color, value]: blocks)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
            ImGui::Button("##TimingVisualizer", {10, 10});
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
        }

        // End the window
        ImGui::End();
    }

    auto inline draw_timing_window_config_tab(bm2dx::timing_t& window, bool enabled) -> void
    {
        ImGui::PushItemWidth(125);

        // Column headers
        ImGui::SameLine(200);
        ImGui::TextColored({0.f, 0.518f, 1.f, 1.f}, "EARLY");
        ImGui::SameLine(330);
        ImGui::TextColored({1.f, 0.f, 0.f, 1.f}, "LATE");

        // Timing visualizer
        draw_timing_visualizer(window);

        // Window settings
        ImGui::BeginDisabled(!enabled);

        auto const bad_size = std::fabs(window.early_bad) + std::fabs(window.late_bad);
        auto const good_size = std::fabs(window.early_good) + std::fabs(window.late_good);
        auto const great_size = std::fabs(window.early_great) + std::fabs(window.late_great);
        auto const pgreat_size = std::fabs(window.early_pgreat) + std::fabs(window.late_pgreat);

        ImGui::TextColored({1.f, 0.5f, 0.f, 1.f}, "POOR");
        ImGui::SameLine();
        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "(%.2f ms)", bad_size);
        ImGui::SameLine(200);
        ImGui::InputFloat("##EarlyBad", &window.early_bad, increment);
        ImGui::SameLine(330);
        ImGui::InputFloat("##LateBad", &window.late_bad, increment);

        ImGui::TextColored({1.f, 1.f, 0.f, 1.f}, "GOOD");
        ImGui::SameLine();
        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "(%.2f ms)", good_size);
        ImGui::SameLine(200);
        ImGui::InputFloat("##EarlyGood", &window.early_good, increment);
        ImGui::SameLine(330);
        ImGui::InputFloat("##LateGood", &window.late_good, increment);

        ImGui::TextColored({0.f, 1.f, 1.f, 1.f}, "GREAT");
        ImGui::SameLine();
        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "(%.2f ms)", great_size);
        ImGui::SameLine(200);
        ImGui::InputFloat("##EarlyGreat", &window.early_great, increment);
        ImGui::SameLine(330);
        ImGui::InputFloat("##LateGreat", &window.late_great, increment);

        ImGui::TextColored({1.f, 1.f, 1.f, 1.f}, "PGREAT");
        ImGui::SameLine();
        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "(%.2f ms)", pgreat_size);
        ImGui::SameLine(200);
        ImGui::InputFloat("##EarlyPGreat", &window.early_pgreat, increment);
        ImGui::SameLine(330);
        ImGui::InputFloat("##LatePGreat", &window.late_pgreat, increment);

        ImGui::EndDisabled();
        ImGui::PopItemWidth();
    }

    auto draw_player_config_tab(std::uint8_t player) -> void
    {
        auto const name = (player == 0 ? "P1/LEFT": "P2/RIGHT");

        if (!ImGui::BeginTabItem(name))
            return;

        // Player-specific data
        auto& override = timing_modifier::override[player];
        auto& enabled_keys = timing_modifier::enabled_keys[player];
        auto& enabled_scratch = timing_modifier::enabled_scratch[player];
        auto& copy_from_chart = timing_modifier::copy_from_chart[player];

        // Alter Timing Windows
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Alter Judgement Timing");
                ImGui::SameLine();
                if (timing_modifier::is_cheating(player))
                {
                    ImGui::SameLine();
                    ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                }
                ImGui::SameLine(300);
                if (ImGui::Checkbox("Keys##AlterKeys", &enabled_keys))
                    copy_from_chart &= !(enabled_keys || enabled_scratch);
                ImGui::SameLine(360);
                if (ImGui::Checkbox("Scratch##AlterScratch", &enabled_scratch))
                    copy_from_chart &= !(enabled_keys || enabled_scratch);
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Use customized judgement timing windows");
        }

        // Copy from Chart
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
            {
                ImGui::Text("Copy from Chart");
                ImGui::SameLine(300);
                ImGui::BeginDisabled(enabled_keys || enabled_scratch);
                ImGui::Checkbox("##CopyFromChart", &copy_from_chart);
                ImGui::EndDisabled();
            }
            ImGui::PopStyleVar();
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Load timing windows from the current chart");
        }

        // Timing Windows
        ImGui::BeginTabBar("Type");

        if (ImGui::BeginTabItem("Keyboard"))
        {
            draw_timing_window_config_tab(override.keys, enabled_keys);
            ImGui::Separator();
            if (ImGui::Button("Copy from Turntable"))
                override.keys = override.scratch;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Turntable"))
        {
            draw_timing_window_config_tab(override.scratch, enabled_scratch);
            ImGui::Separator();
            if (ImGui::Button("Copy from Keyboard"))
                override.scratch = override.keys;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        // back button
        if (ImGui::Button("Return to main menu"))
            visible = false;

        ImGui::EndTabItem();
    }

	auto render() -> void
    {
		ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f}, 0, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({485, 290});

		ImGui::Begin("Configuration :: Timing Modifier", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        if (bm2dx::state->play_style == 0 && !bm2dx::state->p1_active && !bm2dx::state->p2_active)
        {
            ImGui::Text("Options will appear here once a player is active");

            if (ImGui::Button("Return to main menu"))
                visible = false;

            return ImGui::End();
        }

		ImGui::BeginTabBar("Timing Modifier");

        if (bm2dx::state->p1_active || bm2dx::state->play_style == 1) draw_player_config_tab(0);
        if (bm2dx::state->p2_active || bm2dx::state->play_style == 1) draw_player_config_tab(1);

        ImGui::EndTabBar();
		ImGui::End();
	}
}