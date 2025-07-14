#pragma once

#include <cstdint>

extern "C"
{
    auto Version() -> const char*;
    auto Score_Set(const char* id, std::uint8_t player, std::int32_t clear, std::int32_t miss, std::int32_t score) -> void;
    auto Score_Clear() -> void;
    auto Score_SetRival(const char* id, std::uint8_t rival, std::int32_t clear, std::int32_t miss, std::int32_t score) -> void;
    auto Score_ClearRival() -> void;
    auto Chart_Exists(const char* id) -> bool;
    auto Is_Custom_Chart(std::uint8_t player) -> bool;
    auto Get_Last_Chart_ID(std::uint8_t player) -> const char*;
    auto Get_Last_Note_Count(std::uint8_t player) -> std::uint32_t;
}