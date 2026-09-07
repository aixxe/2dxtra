#include "all_scratch.h"

#include <array>
#include <cstring>
#include <vector>

#include "game.h"
#include "urafumen.h"

namespace all_scratch
{
    namespace
    {
        using event = bm2dx::chart_event_t;
        using enum bm2dx::chart_event_type;

        constexpr std::int8_t SCRATCH_COLUMN = 7;
        constexpr std::size_t COLUMN_COUNT = 8;
        constexpr std::int32_t MIN_SCRATCH_GAP = 50;

        constexpr std::int16_t NO_SAMPLE = -1;
        constexpr std::int32_t NO_INDEX = -1;

        auto build_next_scratch_table(const std::vector<event>& events,
            const bm2dx::chart_event_type note_type) -> std::vector<std::int32_t>
        {
            auto const count = static_cast<std::int32_t>(events.size());
            auto next = std::vector<std::int32_t>(events.size() + 1, NO_INDEX);

            for (auto index = count - 1; index >= 0; --index)
            {
                auto const& candidate = events[static_cast<std::size_t>(index)];

                next[static_cast<std::size_t>(index)] =
                    (candidate.type == note_type && candidate.parameter == SCRATCH_COLUMN)
                        ? index
                        : next[static_cast<std::size_t>(index) + 1];
            }

            return next;
        }

        auto scratch_follows_closely(const std::vector<event>& events,
            const std::vector<std::int32_t>& next_scratch, const std::int32_t index,
            const std::int32_t note_offset) -> bool
        {
            auto const found = next_scratch[static_cast<std::size_t>(index) + 1];

            if (found == NO_INDEX)
                return false;

            return (events[static_cast<std::size_t>(found)].offset - note_offset) < MIN_SCRATCH_GAP;
        }

        auto convert_side(const std::vector<event>& source, std::vector<event>& converted,
            const int player) -> void
        {
            auto const note_type = static_cast<bm2dx::chart_event_type>(player);
            auto const sample_type = static_cast<bm2dx::chart_event_type>(player + 2);

            auto const next_scratch = build_next_scratch_table(source, note_type);

            converted.clear();
            converted.reserve(source.size() + 8);

            auto column_sample = std::array<std::int16_t, COLUMN_COUNT> {};
            column_sample.fill(NO_SAMPLE);

            auto scratch_sample = NO_SAMPLE;
            auto scratch_free_from = std::int32_t { 0 };

            auto const count = static_cast<std::int32_t>(source.size());

            for (auto index = std::int32_t { 0 }; index < count; ++index)
            {
                auto record = source[static_cast<std::size_t>(index)];

                if (record.type == END_OF_SONG)
                {
                    converted.push_back(record);
                    break;
                }

                if (record.type == sample_type)
                {
                    if (record.parameter >= 0 && record.parameter < static_cast<int>(COLUMN_COUNT))
                        column_sample[static_cast<std::size_t>(record.parameter)] = record.value;

                    if (record.parameter < SCRATCH_COLUMN)
                        converted.push_back(record);

                    continue;
                }

                if (record.type != note_type)
                {
                    converted.push_back(record);
                    continue;
                }

                if (record.parameter >= SCRATCH_COLUMN)
                {
                    auto const wanted = column_sample[SCRATCH_COLUMN];

                    if (wanted != scratch_sample && wanted != NO_SAMPLE)
                    {
                        auto const gap = record.offset - scratch_free_from;

                        converted.push_back(event { scratch_free_from + (gap >> 1),
                            sample_type, SCRATCH_COLUMN, wanted });

                        scratch_sample = wanted;
                    }

                    scratch_free_from = record.offset + record.value;
                    converted.push_back(record);
                    continue;
                }

                if (record.value == 0 && record.offset > scratch_free_from)
                {
                    auto const gap = record.offset - scratch_free_from;

                    if (gap >= MIN_SCRATCH_GAP &&
                        !scratch_follows_closely(source, next_scratch, index, record.offset))
                    {
                        auto const wanted = record.parameter >= 0
                            ? column_sample[static_cast<std::size_t>(record.parameter)]
                            : NO_SAMPLE;

                        if (wanted != NO_SAMPLE && wanted != scratch_sample)
                        {
                            converted.push_back(event { scratch_free_from + (gap >> 1),
                                sample_type, SCRATCH_COLUMN, wanted });

                            scratch_sample = wanted;
                        }

                        record.parameter = SCRATCH_COLUMN;
                        scratch_free_from = record.offset;
                    }
                }

                converted.push_back(record);
            }
        }

        auto apply_all_scratch(std::vector<event> events) -> std::vector<event>
        {
            auto converted = std::vector<event> {};

            for (auto player = 0; player < 2; ++player)
            {
                convert_side(events, converted, player);
                events.swap(converted);
            }

            return events;
        }
    }

    auto convert_in_place(std::uint8_t* buffer, const std::size_t capacity)
        -> std::size_t
    {
        if (buffer == nullptr || capacity < urafumen::EVENT_SIZE)
            return 0;

        auto parsed = urafumen::parse_events_until_eos({ buffer, capacity });
        if (!parsed)
            return 0;

        auto const converted = apply_all_scratch(std::move(*parsed));
        auto const written = converted.size() * urafumen::EVENT_SIZE;

        if (written > capacity)
            return 0;

        auto* cursor = buffer;

        for (auto const& record: converted)
        {
            bm2dx::write_chart_event(cursor, record);
            cursor += urafumen::EVENT_SIZE;
        }

        return written;
    }
}
