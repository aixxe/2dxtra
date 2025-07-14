#include <random>
#include "regular_speed.h"

namespace iidxtra::keysound_switch
{
    auto override_type = 0;
    auto mute_bgm = false;

    auto reset() -> void
    {
        override_type = 0;
        mute_bgm = false;
    }

    auto remove_bgm_keysounds(std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
        for (auto& event: buffer)
            if (event.type == 7) // background note
                event.value = 0;
    }

    auto remove_note_keysounds(std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
        for (auto& event: buffer)
            if (event.type == 2 || event.type == 3) // sample changes
                event.value = 0;
    }

    auto randomize_keysounds(std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
        // collect all keysounds
        auto keysounds = std::vector<decltype(bm2dx::chart_event_t::value)> {};

        for (auto const& event: buffer)
            if (event.type == 2 || event.type == 3)
                keysounds.emplace_back(event.value);

        // now pick randomly
        auto rng = std::default_random_engine { std::random_device {} () };
        std::ranges::shuffle(keysounds, rng);

        auto it = keysounds.begin();

        for (auto& event: buffer)
        {
            if (event.type == 2 || event.type == 3)
            {
                event.value = *it;
                it = std::next(it);
            }
        }
    }

    auto shuffle_keysounds(std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
        // filter events to only include playable note sample changes
        auto keysounds = std::vector<bm2dx::chart_event_t*> {};

        for (auto& event: buffer)
            if (event.type == 2 || event.type == 3)
                keysounds.emplace_back(&event);

        // randomly select a variety of keysounds to modify
        auto rng = std::default_random_engine { std::random_device {} () };
        auto dist = std::bernoulli_distribution { 0.5 }; // 50% change of rotating

        // slightly re-arrange the keysounds
        for (auto it = keysounds.begin() + 1; *it != keysounds.back(); ++it)
        {
            if (!dist(rng))
                continue;

            auto prev = std::prev(it);
            auto next = std::next(it);

            auto prev_keysound = (*prev)->value;
            auto current_keysound = (*it)->value;
            auto next_keysound = (*next)->value;

            (*prev)->value = current_keysound;
            (*it)->value = next_keysound;
            (*next)->value = prev_keysound;
        }
    }

	auto mutate(std::uint8_t, std::vector<bm2dx::chart_event_t>& buffer) -> void
    {
        // this is independent of the override type
        if (mute_bgm)
            remove_bgm_keysounds(buffer);

        // check override types
        if (override_type == 0)
            return;

        if (override_type == 1)
            return remove_note_keysounds(buffer);

        if (override_type == 2)
            return shuffle_keysounds(buffer);

        if (override_type == 3)
            return randomize_keysounds(buffer);
	}
}