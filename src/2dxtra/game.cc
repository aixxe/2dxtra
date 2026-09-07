#include "game.h"
#include "util/scoped_page_permissions.h"

namespace bm2dx
{
	std::uint8_t* base = nullptr;
	std::optional<offsets> addr = std::nullopt;

	CApplicationConfig* config = nullptr;
	state_t* state = nullptr;
	InputManagerIIDX* input_manager = nullptr;
	play_state_t* play_state = nullptr;
	play_session_t* play_session = nullptr;
	dead_state_t* dead_state = nullptr;
	random_data_t* random_data = nullptr;
	game_score_t* scores[2] = { nullptr, nullptr };
	game_score_t* rival_scores[2] = { nullptr, nullptr };
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

		if (!addr.has_value())
			return false;

		if (addr->MAX_ENTRIES == 0)
			return false;

		config = reinterpret_cast<CApplicationConfig* (*) ()>(addr->GET_APP_CONFIG)();
		state = reinterpret_cast<state_t*>(addr->GAME_STATE);
		input_manager = reinterpret_cast<InputManagerIIDX*>(addr->INPUT_MANAGER);
		play_state = reinterpret_cast<play_state_t*>(addr->PLAY_STATE);
		play_session = reinterpret_cast<play_session_t*>(addr->PLAY_SESSION);
		dead_state = reinterpret_cast<dead_state_t*>(addr->DEAD_STATE);
		random_data = reinterpret_cast<random_data_t*>(addr->RANDOM_DATA);
		scores[0] = reinterpret_cast<game_score_t*>(addr->SCORES_P1);
		scores[1] = reinterpret_cast<game_score_t*>(addr->SCORES_P2);
		rival_scores[0] = reinterpret_cast<game_score_t*>(addr->RIVAL_SCORES_P1);
		rival_scores[1] = reinterpret_cast<game_score_t*>(addr->RIVAL_SCORES_P2);

		return config != nullptr;
	}

	auto max_entries() -> std::size_t
		{ return addr ? static_cast<std::size_t>(addr->MAX_ENTRIES): 0; }

	auto player_scores_size() -> std::size_t
		{ return 2 * max_entries() * sizeof(game_score_t); }

	auto rival_scores_size() -> std::size_t
		{ return 2 * MAX_RIVALS * max_entries() * sizeof(game_score_t); }

	auto player_score(const int player, const play_style style, const std::uint32_t music_id) -> game_score_t*
	{
		if (player < 0 || player > 1 || scores[player] == nullptr || music_id >= max_entries())
			return nullptr;

		return scores[player] + static_cast<std::size_t>(style) * max_entries() + music_id;
	}

	auto rival_score(const int player, const int rival, const play_style style,
	                 const std::uint32_t music_id) -> game_score_t*
	{
		if (player < 0 || player > 1 || rival < 0 || rival >= MAX_RIVALS)
			return nullptr;

		if (rival_scores[player] == nullptr || music_id >= max_entries())
			return nullptr;

		auto const slot = (static_cast<std::size_t>(style) * MAX_RIVALS + rival) * max_entries();

		return rival_scores[player] + slot + music_id;
	}

	auto music_first(music_data_t* data) -> music_entry_t*
	{
		if (data == nullptr)
			return nullptr;

		auto* const table = reinterpret_cast<std::uint8_t*>(data) + offsetof(music_data_t, indexes);

		return reinterpret_cast<music_entry_t*>(table + max_entries() * sizeof(std::int32_t));
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
		FlushInstructionCache(GetCurrentProcess(), addr->SOFT_REV_PATCH, sizeof(bytes));
#endif
	}
}