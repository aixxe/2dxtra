#pragma once

#include "game.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace urafumen
{
    using event = bm2dx::chart_event_t;
    using event_type = bm2dx::chart_event_type;

    enum class column: std::uint8_t
    {
        key1 = 0,
        key2 = 1,
        key3 = 2,
        key4 = 3,
        key5 = 4,
        key6 = 5,
        key7 = 6,
        scratch = 7,
    };

    auto inline constexpr EVENT_SIZE = std::size_t { 8 };
    auto inline constexpr SCRATCH_COL = static_cast<std::int32_t>(column::scratch);

    struct result
    {
        std::vector<event> events;
        int baseline = 0;
        int generated = 0;
    };

    auto parse_events(std::span<const std::uint8_t> data) -> std::vector<event>;
    auto pack_events(std::span<const event> events) -> std::vector<std::uint8_t>;

    auto parse_events_until_eos(std::span<const std::uint8_t> data) -> std::optional<std::vector<event>>;
    auto convert(std::span<const event> chart, int player, bool kichiku) -> result;
    auto convert_in_place(std::uint8_t* buffer, std::size_t capacity, int player, bool kichiku) -> std::size_t;

    auto sha256_hex(const std::uint8_t* data, std::size_t len) -> std::string;
}
