cmake_minimum_required(VERSION 3.20)

project(2dxtra VERSION 5.1.0.0 LANGUAGES CXX)

include(cmake/get_cpm.cmake)
include(cmake/GetGitRevisionDescription.cmake)

get_git_head_revision(GIT_REFSPEC GIT_COMMIT_HASH)

string(REPLACE "refs/heads/" "" GIT_BRANCH_NAME ${GIT_REFSPEC})
string(SUBSTRING ${GIT_COMMIT_HASH} 0 8 GIT_COMMIT_HASH_SHORT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_EXCEPTIONS=0 -DUNICODE")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Compile time options
set(BLOCK_ALL_SCORE_SAVE     0)
set(BLOCK_FORCE_RANDOM_SAVE  0)
set(FORCE_EVENT_MODE_ENABLED 0)

# Version information
set(IS_PRE_RELEASE 0)

CPMAddPackage(
    NAME               zydis
    GIT_TAG            v4.0.0
    GITHUB_REPOSITORY  zyantific/zydis
    OPTIONS            "ZYDIS_MINIMAL_MODE ON" "ZYDIS_FEATURE_ENCODER OFF" "ZYDIS_FEATURE_FORMATTER OFF"
                       "ZYDIS_FEATURE_AVX512 OFF" "ZYDIS_FEATURE_KNC OFF" "ZYDIS_FEATURE_SEGMENT OFF"
)

CPMAddPackage(
    NAME              safetyhook
    GIT_TAG           v0.4.1
    GITHUB_REPOSITORY cursey/safetyhook
)

CPMAddPackage(
    NAME              fmt
    GIT_TAG           10.2.1
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS           "FMT_INSTALL OFF"
)

CPMAddPackage(
    NAME              picosha2
    GIT_TAG           v1.0.0
    GITHUB_REPOSITORY okdshin/picosha2
)

CPMAddPackage(
    NAME              imgui
    GIT_TAG           v1.90.8
    GITHUB_REPOSITORY ocornut/imgui
)

CPMAddPackage(
    NAME              minhook
    GIT_TAG           master
    GITHUB_REPOSITORY TsudaKageyu/minhook
)

CPMAddPackage(
    NAME              rapidxml
    URL               https://netcologne.dl.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
    URL_HASH          SHA256=c3f0b886374981bb20fabcf323d755db4be6dba42064599481da64a85f5b3571
)

set(SOURCE_FILES
    src/2dxtra/api.cc
    src/2dxtra/log.cc
    src/2dxtra/init.cc
    src/2dxtra/game.cc
    src/2dxtra/input.cc
    src/2dxtra/chart_set.cc
    src/2dxtra/score_set.cc
    src/2dxtra/util/memory.cc
    src/2dxtra/gui/gui.cc
    src/2dxtra/gui/loader.cc
    src/2dxtra/gui/log_window.cc
    src/2dxtra/gui/main_window.cc
    src/2dxtra/gui/unrandomizer_window.cc
    src/2dxtra/gui/cn_transformer_window.cc
    src/2dxtra/gui/timing_modifier_window.cc
    src/2dxtra/gui/autoretry_window.cc
    src/2dxtra/hooks/mdata_load_hook.cc
    src/2dxtra/hooks/chart_load_hook.cc
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

if (NOT DEFINED DXSDK_DIR)
  set(DXSDK_DIR $ENV{DXSDK_DIR})
endif()

if (EXISTS ${DXSDK_DIR})
  message(STATUS "Using DirectX SDK at ${DXSDK_DIR}")
  target_link_directories(${PROJECT_NAME} PUBLIC ${DXSDK_DIR}/Lib/x64)
else()
  CPMAddPackage(
      NAME              minidx9
      GIT_TAG           7751cf73f5c06f1be21f5f31c3e2d9a7bacd3a93
      GITHUB_REPOSITORY hrydgard/minidx9
  )
  target_link_directories(${PROJECT_NAME} PUBLIC ${minidx9_SOURCE_DIR}/Lib/x64)
endif()

target_sources(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

file(GLOB OFFSETS src/2dxtra/offsets/*.cc)

foreach (OFFSET_PATH ${OFFSETS})
    message(STATUS "Including version support file: ${OFFSET_PATH}...")

    file(READ "${OFFSET_PATH}" CONTENT)
    set(ALL_OFFSETS "${ALL_OFFSETS}\n${CONTENT}")
endforeach ()

configure_file(src/2dxtra/meta.h.in macros/meta.h @ONLY)
configure_file(src/2dxtra/offsets.h.in macros/offsets.h @ONLY)

target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
)

target_link_libraries(${PROJECT_NAME} safetyhook minhook fmt d3dx9.lib)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${imgui_SOURCE_DIR}
    ${rapidxml_SOURCE_DIR}
    ${DXSDK_DIR}/Include
    ${CMAKE_CURRENT_BINARY_DIR}/macros
)