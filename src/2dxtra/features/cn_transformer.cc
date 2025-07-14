#include <array>
#include <random>
#include "../hooks/score_invalidator_hook.h"
#include "cn_transformer.h"

namespace iidxtra::cn_transformer
{
    using event_store = std::vector<std::tuple<bm2dx::chart_event_t*, std::int16_t>>;

    auto enabled_p1 = false;
    auto enabled_p2 = false;

    auto long_note_percentage_p1 = 5;
    auto long_note_percentage_p2 = 5;

    auto max_cn_duration_p1 = -1;
    auto max_cn_duration_p2 = -1;

    auto allow_backspin_scratch_p1 = false;
    auto allow_backspin_scratch_p2 = false;

    auto reset() -> void
    {
        enabled_p1 = false;
        enabled_p2 = false;
        long_note_percentage_p1 = 5;
        long_note_percentage_p2 = 5;
        max_cn_duration_p1 = -1;
        max_cn_duration_p2 = -1;
        allow_backspin_scratch_p1 = false;
        allow_backspin_scratch_p2 = false;
    }

    // todo: make this actually fun to use (IMPOSSIBLE)
    auto inline do_transform(const std::uint8_t player, const event_store& column) -> void
    {
        auto const chance = (player == 0 ? long_note_percentage_p1: long_note_percentage_p2);
        auto const max_duration = (player == 0 ? max_cn_duration_p1: max_cn_duration_p2);

        auto rng = std::default_random_engine { std::random_device {} () };

        using rand_between = std::uniform_int_distribution<>;

        for (auto current = column.begin(); current != column.end(); ++current)
        {
            auto next = std::next(current);

            if (next == column.end())
                break;

            auto const this_note = std::get<0>(*current);
            auto const next_note = std::get<0>(*next);

            // skip instructions at offsets considered invalid
            if (this_note->offset == 0 || this_note->offset == INT_MAX)
                continue;

            // this is a note we just merged, ignore it
            if (this_note->type == 7)
                continue;

            // make sure this would satisfy the max duration setting
            auto const duration = static_cast<std::int16_t>
                (next_note->offset - this_note->offset);

            if (max_duration > -1 && duration > max_duration)
                continue;

            // randomly decide whether to actually make this a CN
            if (rand_between(0, 100) (rng) >= chance)
                continue;

            // extend this note to the next note...
            this_note->value = duration;

            // ...then turn next node into a background sound
            next_note->type = 7;
            next_note->parameter = 8;
            next_note->value = std::get<1>(*next);

            // advance iterator again so we skip the next note
            ++current;
        }
    }

    auto mutate(const std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
		auto const enabled = (player == 0 ? enabled_p1: enabled_p2);
        auto const is_dp = (bm2dx::state->play_style == 1);

		if (!enabled)
		{
			if (!is_dp)
				return;

			if (!enabled_p1 && !enabled_p2)
				return;
		}

        auto columns_p1 = std::array<event_store, 8> {};
        auto columns_p2 = std::array<event_store, 8> {};

        auto last_loaded_samples_p1 = std::array<int, 8> {};
        auto last_loaded_samples_p2 = std::array<int, 8> {};

        for (auto& event: buffer)
        {
            auto is_valid_key_p1 = false;
            auto is_valid_key_p2 = false;

            if (is_dp)
            {
                // in DP we just simply use the config as-is
                is_valid_key_p1 = enabled_p1 && (event.parameter >= 0 && event.parameter <= 7);
                is_valid_key_p2 = enabled_p2 && (event.parameter >= 0 && event.parameter <= 7);

                if (!allow_backspin_scratch_p1)
                    is_valid_key_p1 = (is_valid_key_p1 && event.parameter != 7);
                else if (!allow_backspin_scratch_p2)
                    is_valid_key_p2 = (is_valid_key_p2 && event.parameter != 7);
            }
            else
            {
                // for SP, P2 settings need to be set on P1
                is_valid_key_p1 = (event.parameter >= 0 && event.parameter <= 7);
                is_valid_key_p2 = false;

                if ((player == 0 && !allow_backspin_scratch_p1) || (player == 1 && !allow_backspin_scratch_p2))
                    is_valid_key_p1 = (is_valid_key_p1 && event.parameter != 7);
            }

            if (event.type == 2)
                last_loaded_samples_p1[event.parameter] = event.value;
            else if (event.type == 3)
                last_loaded_samples_p2[event.parameter] = event.value;

            if (event.type == 0 && is_valid_key_p1)
                columns_p1[event.parameter].emplace_back(&event, last_loaded_samples_p1[event.parameter]);
            else if (event.type == 1 && is_valid_key_p2)
                columns_p2[event.parameter].emplace_back(&event, last_loaded_samples_p2[event.parameter]);
        }

        // notes are only separated into P1/P2 for DP. in SP all the notes will be in columns_p1
        // to ensure the correct settings are used for P2 players, provide the real player ID here
        for (auto const& column: columns_p1)
            do_transform(!is_dp ? player: 0, column);

        for (auto const& column: columns_p2)
            do_transform(1, column);

        // prevent scores from saving
        if (bm2dx::state->play_style == 1)
            score_invalidator_hook::invalidate(bm2dx::state->p1_active ? 0: 1);
        else
            score_invalidator_hook::invalidate(player);
    }
}