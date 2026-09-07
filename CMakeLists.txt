cmake_minimum_required(VERSION 3.20)

project(2dxtra VERSION 6.0.0.0 LANGUAGES CXX)

include(cmake/GetGitRevisionDescription.cmake)

get_git_head_revision(GIT_REFSPEC GIT_COMMIT_HASH)

string(REPLACE "refs/heads/" "" GIT_BRANCH_NAME ${GIT_REFSPEC})
string(SUBSTRING ${GIT_COMMIT_HASH} 0 8 GIT_COMMIT_HASH_SHORT)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Compile time options
set(BLOCK_ALL_SCORE_SAVE     0)
set(BLOCK_FORCE_RANDOM_SAVE  0)
set(FORCE_EVENT_MODE_ENABLED 0)

# Version information
set(IS_PRE_RELEASE 0)

# vcpkg dependencies
find_package(fmt CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(safetyhook CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_library(RUBBERBAND_LIBRARY rubberband REQUIRED)
find_library(SAMPLERATE_LIBRARY samplerate REQUIRED)
find_path(RUBBERBAND_INCLUDE_DIR rubberband/RubberBandStretcher.h REQUIRED)

set(SOURCE_FILES
    src/2dxtra/api.cc
    src/2dxtra/log.cc
    src/2dxtra/init.cc
    src/2dxtra/game.cc
    src/2dxtra/input.cc
    src/2dxtra/chart_set.cc
    src/2dxtra/score_set.cc
    src/2dxtra/database.cc
    src/2dxtra/urafumen.cc
    src/2dxtra/all_scratch.cc
    src/2dxtra/gui/gui.cc
    src/2dxtra/gui/loader.cc
    src/2dxtra/gui/log_window.cc
    src/2dxtra/gui/main_window.cc
    src/2dxtra/gui/unrandomizer_window.cc
    src/2dxtra/gui/cn_transformer_window.cc
    src/2dxtra/gui/timing_modifier_window.cc
    src/2dxtra/gui/autoretry_window.cc
    src/2dxtra/hooks/mdata_load_hook.cc
    src/2dxtra/hooks/boot_screen_hook.cc
    src/2dxtra/hooks/chart_load_hook.cc
    src/2dxtra/hooks/chart_analyze_hook.cc
    src/2dxtra/hooks/input_hook.cc
    src/2dxtra/hooks/network_hook.cc
    src/2dxtra/hooks/reset_state_hook.cc
    src/2dxtra/hooks/result_title_hook.cc
    src/2dxtra/hooks/dan_select_hook.cc
    src/2dxtra/hooks/music_select_hook.cc
    src/2dxtra/hooks/mselect_genre_hook.cc
    src/2dxtra/hooks/stage_result_hook.cc
    src/2dxtra/hooks/renderer_hook.cc
    src/2dxtra/hooks/score_invalidator_hook.cc
    src/2dxtra/hooks/card_out_hook.cc
    src/2dxtra/hooks/play_field_load_hook.cc
    src/2dxtra/hooks/judge_timing_hook.cc
    src/2dxtra/hooks/attract_randomizer_hook.cc
    src/2dxtra/features/cn_transformer.cc
    src/2dxtra/features/keysound_switch.cc
    src/2dxtra/features/autoplay.cc
    src/2dxtra/features/regular_speed.cc
    src/2dxtra/features/chart_speed.cc
    src/2dxtra/features/cn_override.cc
    src/2dxtra/features/chart_loader.cc
    src/2dxtra/features/unrandomizer.cc
    src/2dxtra/features/scratch_swap.cc
    src/2dxtra/features/play_visuals.cc
    src/2dxtra/features/timing_modifier.cc
    src/2dxtra/features/autoretry.cc
    res/2dxtra.def
    res/2dxtra.rc
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "2dxtra" PREFIX "")

file(GLOB OFFSETS src/2dxtra/offsets/*.cc)

foreach (OFFSET_PATH ${OFFSETS})
    message(STATUS "Including version support file: ${OFFSET_PATH}...")

    file(READ "${OFFSET_PATH}" CONTENT)
    set(ALL_OFFSETS "${ALL_OFFSETS}\n${CONTENT}")
endforeach ()

configure_file(src/2dxtra/meta.h.in macros/meta.h @ONLY)
configure_file(src/2dxtra/offsets.h.in macros/offsets.h @ONLY)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
)

target_link_libraries(${PROJECT_NAME} safetyhook::safetyhook minhook::minhook fmt::fmt imgui::imgui unofficial::sqlite3::sqlite3 zstd::libzstd dwmapi ${RUBBERBAND_LIBRARY} ${SAMPLERATE_LIBRARY})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/macros
    ${RUBBERBAND_INCLUDE_DIR}
)