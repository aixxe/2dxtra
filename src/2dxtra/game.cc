#include "game.h"
#include "util/scoped_page_permissions.h"

namespace bm2dx
{
	std::uint8_t* base = nullptr;
	std::optional<offsets> addr = std::nullopt;

	CApplicationConfig* config = nullptr;
	state_t* state = nullptr;
	bool* in_gameplay = nullptr;
	random_data_t* random_data = nullptr;
	player_scores_t* scores[2] = { nullptr, nullptr };
	rival_scores_t* rival_scores[2] = { nullptr, nullptr };
    play_field_t* play_field = nullptr;
	std::unordered_map<std::uint32_t, music_entry_t*> music_map {}; // filled in by mdata_load_hook

	auto resolve() -> bool
	{
		auto module = static_cast<HMODULE>(nullptr);

        for (auto&& name: {"bm2dx.dll", "bm2dx_omni.dll"})
        {
            module = GetModuleHandleA(name);

            if (module)
                break;
        }

		if (module == nullptr)
			return false;

		base = reinterpret_cast<std::uint8_t*>(module);
		addr = resolve_offsets(base);

		config = reinterpret_cast<CApplicationConfig* (*) ()>(addr->GET_APP_CONFIG)();
		state = reinterpret_cast<state_t*>(addr->GAME_STATE);
		in_gameplay = reinterpret_cast<bool*>(addr->IN_GAMEPLAY);
		random_data = reinterpret_cast<random_data_t*>(addr->RANDOM_COLUMNS);
		scores[0] = reinterpret_cast<player_scores_t*>(addr->SCORES_P1);
		scores[1] = reinterpret_cast<player_scores_t*>(addr->SCORES_P2);
		rival_scores[0] = reinterpret_cast<rival_scores_t*>(addr->RIVAL_SCORES_P1);
		rival_scores[1] = reinterpret_cast<rival_scores_t*>(addr->RIVAL_SCORES_P2);

		return true;
	}

	auto set_soft_rev(std::uint8_t rev) -> void
	{
		addr->GAME_MODEL[8] = rev;

#ifdef NDEBUG
		// required for propagating to network calls
		std::uint8_t bytes[] = { 0xC6, 0x47, 0x05, rev, // mov byte ptr [rdi+05], 45 ('E')
			                     0x90 };                // nop

		auto guard = iidxtra::util::scoped_page_permissions
			{ addr->SOFT_REV_PATCH, sizeof(bytes), PAGE_EXECUTE_READWRITE };

		CopyMemory(addr->SOFT_REV_PATCH, bytes, sizeof(bytes));
#endif
	}
}