#include <meta.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <fmt/format.h>
#include "gui.h"
#include "loader.h"
#include "main_window.h"
#include "unrandomizer_window.h"
#include "cn_transformer_window.h"
#include "timing_modifier_window.h"
#include "autoretry_window.h"
#include "../game.h"
#include "../chart_set.h"
#include "../features/autoplay.h"
#include "../features/regular_speed.h"
#include "../features/chart_speed.h"
#include "../features/keysound_switch.h"
#include "../features/play_visuals.h"
#include "../features/fast_slow_display.h"
#include "../hooks/fast_slow_hook.h"

namespace iidxtra::gui::main_window
{

    static auto normalize_rate(float rate) -> float
    {
        const auto rounded = std::round(rate * 20.0f) / 20.0f;
        return std::clamp(rounded, chart_speed::rate_min, chart_speed::rate_max);
    }

    static auto modify_rate(float rate, bool add) -> float
    {
        // Snap to the next 0.05 increment in the pressed direction, even between increments.
        // Round down and increment, or round up and decrement.
        const auto scaled = rate * 20.0f;
        const auto modified = add ?
            std::floor(scaled) + 1.0f :
            std::ceil(scaled) - 1.0f;
        return normalize_rate(modified / 20.0f);
    }

    auto render() -> void
    {
		// darken game
        ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, {ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}, IM_COL32(0, 0, 0, 200), 0, 0);

        // popup windows
        if (unrandomizer_window::visible)
            return unrandomizer_window::render();
        if (cn_transformer_window::visible)
            return cn_transformer_window::render();
        if (timing_modifier_window::visible)
            return timing_modifier_window::render();
        if (autoretry_window::visible)
            return autoretry_window::render();

		// window setup
		ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f}, 0, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({550, 450});
		ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoDecoration);

        // usage hints
        {
            draw_hint_text("Use keys [1] and [2] to select an option. Press key [6] to activate", (15 * 0), IM_COL32(255, 255, 255, 150));
            draw_hint_text("With an option selected, move the turntable to adjust the current value", (15 * 1), IM_COL32(255, 255, 255, 150));
            draw_hint_text("To exit this menu, press the [EFFECT] button twice rapidly", (15 * 2), IM_COL32(255, 255, 255, 150));
        }

		// 2dxtra header
		ImGui::TextColored({1.f, (88.f / 255.f), (88.f / 255.f), 1.f}, "2dx");
		ImGui::SameLine(0, 0);
		ImGui::Text("tra");
		ImGui::SameLine(0, 0);
		ImGui::TextColored({0.75f, 0.75f, 0.75f, 1.f}, ", multi-hack for beatmania IIDX");
		ImGui::SameLine(0, 0);

        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, " (v%s-%s@%s)",
                           VERSION_STRING, GIT_COMMIT_HASH_SHORT, GIT_BRANCH_NAME);

        #if FORCE_EVENT_MODE_ENABLED == 1
            ImGui::SameLine(0, 0);
            ImGui::TextColored({1.5f, 1.f, 0.5f, 1.f}, " :: EVENT BUILD");
        #elif IS_PRE_RELEASE == 1
            ImGui::SameLine(0, 0);
            ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " :: PRE-RELEASE BUILD");
        #endif

        // convenience variables for conditional options
        auto is_sp = (bm2dx::state->play_style == 0);
        auto p1_active = (is_sp && bm2dx::state->p1_active) || !is_sp;
        auto p2_active = (is_sp && bm2dx::state->p2_active) || !is_sp;

		// contents
		ImGui::BeginTabBar("Pages");
            if (ImGui::BeginTabItem("Chart"))
            {
                // Loader
                gui::render_loader();

                if (ImGui::CollapsingHeader("Modifiers", ImGuiTreeNodeFlags_DefaultOpen))
                {
                	// Un-randomizer
                	{
                		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("Un-randomizer");
							ImGui::SameLine();
							ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
							ImGui::SameLine(300);

							if (ImGui::Button("Configure##UnrandomizerWindow"))
								unrandomizer_window::visible = true;
						}

						ImGui::PopStyleVar();
                		ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Adjust random lane options");
                	}

                    // Timing Modifier
                	{
                		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("Timing Modifier");
							ImGui::SameLine();
							ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
							ImGui::SameLine(300);

							if (ImGui::Button("Configure##TimingModifierWindow"))
                                timing_modifier_window::visible = true;
						}

						ImGui::PopStyleVar();
                		ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Alter the judgement timing windows");
                	}

                	// Auto Play
					{
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("Auto Play");
							ImGui::SameLine();
							ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");

                            ImGui::BeginDisabled(!p1_active);
                            ImGui::SameLine(300);
                            ImGui::Checkbox("P1##AutoPlay", &autoplay::enabled_p1);
                            ImGui::EndDisabled();

                            ImGui::BeginDisabled(!p2_active);
                            ImGui::SameLine(340);
                            ImGui::Checkbox("P2##AutoPlay", &autoplay::enabled_p2);
                            ImGui::EndDisabled();
						}
						ImGui::PopStyleVar();
						ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Plays the chart so you don't have to");
					}

					// Regular Speed
					{
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("Regular Speed");
							ImGui::SameLine();
							ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
							ImGui::SameLine(300);
							ImGui::Checkbox("Global##RegularSpeed", &regular_speed::enabled);
						}
						ImGui::PopStyleVar();
						ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Removes all BPM changes from a chart");
					}

                    // Chart Speed
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Chart Speed");
                            ImGui::SameLine();
                            ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(!chart_set::switch_enabled);

                            // Slider
                            ImGui::SetNextItemWidth(100);
                            if (ImGui::SliderFloat("##ChartSpeed",
                                &chart_speed::rate, chart_speed::rate_min, chart_speed::rate_max, "x%.2f"))
                            {
                                chart_speed::rate = normalize_rate(chart_speed::rate);
                                chart_speed::rate_previous = chart_speed::rate;
                            }

                            // Decrease
                            const auto button_size = ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
                            ImGui::BeginDisabled(chart_speed::rate <= chart_speed::rate_min);
                            ImGui::SameLine(0, 5.0f);
                            if (ImGui::Button("-##ChartSpeedDown", button_size))
                            {
                                chart_speed::rate = modify_rate(chart_speed::rate, false);
                                chart_speed::rate_previous = chart_speed::rate;
                            }
                            ImGui::EndDisabled();

                            // Increase
                            ImGui::BeginDisabled(chart_speed::rate >= chart_speed::rate_max);
                            ImGui::SameLine(0, 5.0f);
                            if (ImGui::Button("+##ChartSpeedUp", button_size))
                            {
                                chart_speed::rate = modify_rate(chart_speed::rate, true);
                                chart_speed::rate_previous = chart_speed::rate;
                            }
                            ImGui::EndDisabled();

                            // Toggle buttons
                            if (chart_speed::rate == 1.0f)
                            {
                                if (chart_speed::rate_previous != 1.0f)
                                {
                                    const auto label = fmt::format("x{:.2f}##ChartSpeedRestore", chart_speed::rate_previous);
                                    ImGui::SameLine(0, 5.0f);
                                    if (ImGui::Button(label.c_str()))
                                        chart_speed::rate = chart_speed::rate_previous;
                                }
                            }
                            else
                            {
                                ImGui::SameLine(0, 5.0f);
                                if (ImGui::Button("x1.00##ChartSpeedReset"))
                                {
                                    chart_speed::rate_previous = chart_speed::rate;
                                    chart_speed::rate = 1.0f;
                                }
                            }

                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Alter the chart to be faster or slower");
                    }

                    // CN Transformer
                	{
                		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("CN Transformer");
							ImGui::SameLine();
							ImGui::TextColored({1.f, 0.5f, 0.5f, 1.f}, " *");
							ImGui::SameLine(300);

							if (ImGui::Button("Configure##CNTransformerWindow"))
                                cn_transformer_window::visible = true;
						}

						ImGui::PopStyleVar();
                		ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Adjust charge note behavior");
                	}

                    // Auto Retry
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Auto Retry");
                            ImGui::SameLine();
                            ImGui::SameLine(300);

                            if (ImGui::Button("Configure##AutoRetryWindow"))
                                autoretry_window::visible = true;
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Retry when unable to meet pacemaker score");
                    }

					// Keysound Switch
					{
                        auto const options = std::vector<std::tuple<std::string, std::string>> {
                            {"Default", "Use regular key sounds"},
                            {"Muted", "Only background notes will produce sounds"},
                            {"Shuffle", "Slightly re-arrange keysound order"},
                            {"Random!", "Completely randomize all playable keysounds"},
                        };
                        auto const& [mode_text, descriptive_text] = options[keysound_switch::override_type];
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
						{
							ImGui::Text("Keysound Switch");
							ImGui::SameLine(300);
							ImGui::SetNextItemWidth(100);
							ImGui::SliderInt("##KeysoundType", &keysound_switch::override_type, 0, 3, mode_text.c_str());
                            ImGui::SameLine(405);
                            ImGui::Checkbox("Mute BGM", &keysound_switch::mute_bgm);
						}
						ImGui::PopStyleVar();
						ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "%s", descriptive_text.c_str());
					}
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Visuals"))
            {
                if (ImGui::CollapsingHeader("Play", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Dark Mode");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(bm2dx::play_session->in_gameplay);
                            if (ImGui::Checkbox("##DarkMode", &play_visuals::dark_mode))
                                play_visuals::update_dark_mode();
                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Hide most play frame elements");
                    }

                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Disable Measure Lines");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(bm2dx::play_session->in_gameplay);
                            if (ImGui::Checkbox("##HideMeasureLines", &play_visuals::no_measure_lines))
                                play_visuals::update_no_measure_lines();
                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Hide measure lines from the lane");
                    }

                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Disable BPM Gradient");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(bm2dx::play_session->in_gameplay);
                            if (ImGui::Checkbox("##HideBPMGradient", &play_visuals::no_bpm_gradient))
                                play_visuals::update_no_bpm_gradient();
                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Hide the flashing blue bar above the keys");
                    }
                }

                if (ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::BeginDisabled();
                    ImGui::TextWrapped("These options only affect FAST/SLOW indicators "
                                       "and do not affect acutal timing or scoring");
                    ImGui::EndDisabled();

                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Show Milliseconds");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(!fast_slow_hook::available() ||
                                                 bm2dx::play_session->in_gameplay);
                            if (ImGui::Checkbox("##FastSlowMilliseconds",
                                                &fast_slow_display::options.show_milliseconds))
                                fast_slow_display::update();
                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f},
                                           "Show ms value instead of FAST/SLOW");
                    }

                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                        {
                            ImGui::Text("Show FAST/SLOW for PGREAT");
                            ImGui::SameLine(300);
                            ImGui::BeginDisabled(!fast_slow_hook::available() ||
                                                 bm2dx::play_session->in_gameplay);
                            if (ImGui::Checkbox("##FastSlowPGREAT",
                                                &fast_slow_display::options.show_pgreat))
                                fast_slow_display::update();
                            ImGui::EndDisabled();
                        }
                        ImGui::PopStyleVar();
                        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f},
                                           "Requires 120Hz");
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Options"))
            {
                if (ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen))
                {
                	{
                		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                		{
                			ImGui::Text("Menu Lock");
                			ImGui::SameLine(300);
                			ImGui::Checkbox("##PlayLockMenu", &play_lock_state);
                		}
                		ImGui::PopStyleVar();
                		ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Disable menu opening during gameplay");
                	}
                }

                if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
                {
                	{
                		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -5.0f));
                		{
                			ImGui::Text("Event Mode");
                			ImGui::SameLine(300);
                            #if FORCE_EVENT_MODE_ENABLED == 0
                			ImGui::Checkbox("##EventMode", &config_event_mode);
                            #else
                            ImGui::BeginDisabled();
                            ImGui::Checkbox("##EventMode", &config_event_mode);
                            ImGui::EndDisabled();
                            #endif
                		}
                		ImGui::PopStyleVar();
                		ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.f}, "Reset all options to default after card out");
                	}
                }

				ImGui::EndTabItem();
			}
        ImGui::EndTabBar();
        ImGui::End();
	};
}