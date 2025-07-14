#pragma once

#include <optional>
#include <offsets.h>
#include <unordered_map>

namespace bm2dx
{
	// constants
	auto constexpr MAX_RIVALS = 6;
	auto constexpr MAX_ENTRIES = 32000;

	// some request names used by network_hook
	auto constexpr REQUEST_LOBBY_ENTRY = "IIDX31lobby.entry";
	auto constexpr REQUEST_LOBBY_UPDATE = "IIDX31lobby.update";
	auto constexpr REQUEST_LOBBY_DELETE = "IIDX31lobby.delete";
	auto constexpr REQUEST_GAME_SYSTEM_INFO = "IIDX31gameSystem.systemInfo";

    // enums
    enum class pacemaker_type: std::uint8_t
    {
        NO_GRAPH = 0,             // sg_fno
        MY_BEST = 1,              // sg_monly
        RIVAL_1 = 2,              // sg_riva1
        RIVAL_2 = 3,              // sg_riva2
        RIVAL_3 = 4,              // sg_riva3
        RIVAL_4 = 5,              // sg_riva4
        RIVAL_5 = 6,              // sg_riva5
        RIVAL_6 = 7,              // sg_riva6
        RIVAL_NEXT = 8,           // sg_rival_next
        RIVAL_BEST = 9,           // sg_riva_top
        RIVAL_AVERAGE = 10,       // sg_riva_ave
        NATIONAL_BEST = 11,       // sg_altop
        NATIONAL_AVERAGE = 12,    // sg_alave
        PREFECTURE_BEST = 13,     // sg_lotop
        PREFECTURE_AVERAGE = 14,  // sg_loave
        CLASS_BEST = 15,          // sg_dantp
        CLASS_AVERAGE = 16,       // sg_danav
        VENUE_BEST = 17,          // sg_shop_top
        VENUE_NEXT = 18,          // sg_shop_next
        PREVIOUS_GHOST = 19,      // sg_ghost
        PACEMAKER_AAA = 20,       // sg_pacemaker_aaa
        PACEMAKER_AA = 21,        // sg_pacemaker_aa
        PACEMAKER_A = 22,         // sg_pacemaker_a
        PACEMAKER = 23,           // sg_pacemaker
        PACEMAKER_NEXT = 24,      // sg_pacemaker_next
        PACEMAKER_NEXT_PLUS = 25, // sg_pacemaker_nextplus
        VENUE_ROTATE = 26,        // sg_shop_rotate
        PACEMAKER_ROTATE = 27,    // sg_pacemaker_rotate
        RIVAL_ROTATE = 28,        // sg_riva_rotate
        PLAYER_1 = 29,            // sg_player1
        PLAYER_2 = 30,            // sg_player1
    };

    // structures
    struct dead_state_t
    {
        bool p1;
        bool p2;
    };

    struct chart_event_t
	{
		std::int32_t offset = 0; //0x0000
		std::int8_t type = 0; //0x0004
		std::int8_t parameter = 0; //0x0005
		std::int16_t value = 0; //0x0006
	}; static_assert(sizeof(chart_event_t) == 0x8);

    struct chart_buffer_t
    {
        chart_event_t events[0x3000]; //0x0000
        std::uint8_t pad_18000[4]; //0x18000
        std::int32_t p1_note_count; //0x18004
        std::uint8_t pad_18008[1588]; //0x18008
        std::int32_t p2_note_count; //0x1863C
    }; static_assert(sizeof(chart_buffer_t) == 0x18640);

    struct play_note_t
    {
        std::int32_t type; //0x0000
        std::uint8_t pad_0004[12]; //0x0004
        std::int32_t lane1; //0x0010
        std::int32_t lane2; //0x0014
        std::int32_t visible; //0x0018
        std::int32_t offset; //0x001C
        std::uint8_t pad_0020[12]; //0x0020
        std::int32_t end_offset; //0x002C for CNs
        std::uint8_t pad_0030[20]; //0x0030
        std::int32_t hcn; //0x0044
        std::uint8_t pad_0048[8]; //0x0048
    }; static_assert(sizeof(play_note_t) == 0x50);

    struct play_notes_t
    {
        auto begin() { return notes; }
        auto end() { return notes + 16000; }

        play_note_t notes[16000]; //0x0000
    }; static_assert(sizeof(play_notes_t) == 0x138800);

    struct play_field_t
    {
        play_notes_t notes[2]; //0x0000
    }; static_assert(sizeof(play_field_t) == 0x271000);

    struct text_props_t
    {
        std::uint8_t pad_0000[64]; //0x0000
		std::int32_t spacing; //0x0040
		std::uint8_t pad_0044[12]; //0x0044
		std::int32_t h_align; //0x0050
        std::int32_t v_align; //0x0054
        std::uint8_t pad_0058[256]; //0x0058 (extra space for text)
    };

	struct random_data_t
	{
		std::uint32_t columns[2][8]; //0x0000
	}; static_assert(sizeof(random_data_t) == 0x40);

	struct CApplicationConfig
	{
		std::int8_t pad_0000[16]; //0x0000
		std::int32_t cabinet_mode; //0x0010
		std::int32_t target_fps; //0x0014
		std::int8_t pad_0018[4]; //0x0018
		float monitor_check_fps; //0x001C
	}; static_assert(sizeof(CApplicationConfig) == 0x20);

	struct input_t
	{
		std::uint64_t buttons; //0x0008
		std::int8_t pad_0010[8]; //0x0010
		std::int32_t p1_turntable; //0x0018
		std::int32_t p1_turntable_delta; //0x001C
		std::int32_t p2_turntable; //0x0020
		std::int32_t p2_turntable_delta; //0x0024
	}; static_assert(sizeof(input_t) == 0x20);

    struct InputManagerIIDX
	{
		void* vft; //0x0000
		input_t data; //0x0008
	}; static_assert(sizeof(InputManagerIIDX) == 0x28);

	struct notes_radar_t
	{
		std::int32_t notes; //0x0000
		std::int32_t peak; //0x0004
		std::int32_t scratch; //0x0008
		std::int32_t soflan; //0x000C
		std::int32_t charge; //0x0010
		std::int32_t chord; //0x0014
	}; static_assert(sizeof(notes_radar_t) == 0x18);

	struct music_entry_t
	{
		char title[64]; //0x0000
		char title_ascii[64]; //0x0040
		char genre[64]; //0x0080
		char artist[64]; //0x00C0
		std::int32_t texture_title; //0x0100
		std::int32_t texture_artist; //0x0104
		std::int32_t texture_genre; //0x0108
		std::int32_t texture_load; //0x010C
		std::int32_t texture_list; //0x0110
		std::int32_t font_idx; //0x0114
		std::uint16_t game_version; //0x0118
		std::uint16_t other_folder; //0x011A
		std::uint16_t bemani_folder; //0x011C
		std::uint16_t splittable_diff; //0x011E
		std::uint8_t spb_rating; //0x0120
		std::uint8_t spn_rating; //0x0121
		std::uint8_t sph_rating; //0x0122
		std::uint8_t spa_rating; //0x0123
		std::uint8_t spl_rating; //0x0124
		std::uint8_t pad_0125[1]; //0x0125
		std::uint8_t dpn_rating; //0x0126
		std::uint8_t dph_rating; //0x0127
		std::uint8_t dpa_rating; //0x0128
		std::uint8_t dpl_rating; //0x0129
		std::uint8_t pad_012A[6]; //0x012A
		std::uint32_t spb_bpm_max; //0x0130
		std::uint32_t spb_bpm_min; //0x0134
		std::uint32_t spn_bpm_max; //0x0138
		std::uint32_t spn_bpm_min; //0x013C
		std::uint32_t sph_bpm_max; //0x0140
		std::uint32_t sph_bpm_min; //0x0144
		std::uint32_t spa_bpm_max; //0x0148
		std::uint32_t spa_bpm_min; //0x014C
		std::uint32_t spl_bpm_max; //0x0150
		std::uint32_t spl_bpm_min; //0x0154
		std::uint8_t pad_0158[8]; //0x0158
		std::uint32_t dpn_bpm_max; //0x0160
		std::uint32_t dpn_bpm_min; //0x0164
		std::uint32_t dph_bpm_max; //0x0168
		std::uint32_t dph_bpm_min; //0x016C
		std::uint32_t dpa_bpm_max; //0x0170
		std::uint32_t dpa_bpm_min; //0x0174
		std::uint32_t dpl_bpm_max; //0x0178
		std::uint32_t dpl_bpm_min; //0x017C
		std::uint8_t pad_0180[48]; //0x0180
		std::uint32_t spb_note_count; //0x01B0
		std::uint32_t spn_note_count; //0x01B4
		std::uint32_t sph_note_count; //0x01B8
		std::uint32_t spa_note_count; //0x01BC
		std::uint32_t spl_note_count; //0x01C0
		std::uint8_t pad_01C4[4]; //0x01C4
		std::uint32_t dpn_note_count; //0x01C8
		std::uint32_t dph_note_count; //0x01CC
		std::uint32_t dpa_note_count; //0x01D0
		std::uint32_t dpl_note_count; //0x01D4
		std::uint8_t pad_01D8[24]; //0x01D8
		std::int32_t spb_cn_type; //0x01F0
		std::int32_t spn_cn_type; //0x01F4
		std::int32_t sph_cn_type; //0x01F8
		std::int32_t spa_cn_type; //0x01FC
		std::int32_t spl_cn_type; //0x0200
		std::uint8_t pad_0204[4]; //0x0204
		std::int32_t dpn_cn_type; //0x0208
		std::int32_t dph_cn_type; //0x020C
		std::int32_t dpa_cn_type; //0x0210
		std::int32_t dpl_cn_type; //0x0214
		std::uint8_t pad_0218[24]; //0x0218
		notes_radar_t spb_notes_radar; //0x0230 0:B, 1:N, 2:H, 3:A, 4:L
		notes_radar_t spn_notes_radar;
		notes_radar_t sph_notes_radar;
		notes_radar_t spa_notes_radar;
		notes_radar_t spl_notes_radar;
		std::uint8_t pad[0x18];
		notes_radar_t dpn_notes_radar;
		notes_radar_t dph_notes_radar;
		notes_radar_t dpa_notes_radar;
		notes_radar_t dpl_notes_radar;
		std::uint8_t pad_0320[144]; //0x0320
		std::int32_t id; //0x03B0
		std::int32_t volume; //0x03B4
		std::uint8_t pad_03B8[372]; //0x03B8
	}; static_assert(sizeof(music_entry_t) == 0x052C);

	struct music_data_t
	{
		const char magic[4]; //0x0000 "IIDX"
		std::int32_t game_version; //0x0004
		std::int16_t occupied_entries; //0x0008
		std::int16_t maximum_entries; //0x000A
		std::uint8_t pad_000C[64004]; //0x000C
		music_entry_t first; //0xFA10
	}; static_assert(sizeof(music_data_t) == 0xFF3C);

	struct state_t
	{
		std::int32_t game_type; //0x0000
		std::int32_t play_style; //0x0004
		std::int32_t p1_difficulty; //0x0008
		std::int32_t p2_difficulty; //0x000C
		std::int32_t p1_active; //0x0010
		std::int32_t p2_active; //0x0014
		std::uint8_t pad_0018[24]; //0x0018
		music_entry_t* active_music; //0x0030
	}; static_assert(sizeof(state_t) == 0x38);

	struct game_score_t
	{
		std::int32_t score[5]; //0x0000
		std::int32_t miss[5]; //0x0014
		std::int8_t clear[5]; //0x0028
		std::int8_t is_populated; //0x002D
		std::uint8_t pad_002E[2]; //0x002E
	}; static_assert(sizeof(game_score_t) == 0x30);

	struct player_scores_t
	{
		game_score_t sp[MAX_ENTRIES]; //0x0000
		game_score_t dp[MAX_ENTRIES]; //0x177000
	}; static_assert(sizeof(player_scores_t) == 0x2EE000);

    struct rival_scores_t
    {
		game_score_t sp[MAX_RIVALS][MAX_ENTRIES]; //0x0000
		game_score_t dp[MAX_RIVALS][MAX_ENTRIES]; //0x8CA000
    }; static_assert(sizeof(rival_scores_t) == 0x1194000);

    struct timing_t
    {
        // easy adapter between display values and game values
        auto convert(const bool add = true)
        {
            auto constexpr increment = 0.5f;
            early_poor   = add ? (early_poor   + increment): (early_poor   - increment);
            early_bad    = add ? (early_bad    + increment): (early_bad    - increment);
            early_good   = add ? (early_good   + increment): (early_good   - increment);
            early_great  = add ? (early_great  + increment): (early_great  - increment);
            early_pgreat = add ? (early_pgreat + increment): (early_pgreat - increment);
            late_pgreat  = add ? (late_pgreat  + increment): (late_pgreat  - increment);
            late_great   = add ? (late_great   + increment): (late_great   - increment);
            late_good    = add ? (late_good    + increment): (late_good    - increment);
            late_bad     = add ? (late_bad     + increment): (late_bad     - increment);
        }

        float early_poor;
        float early_bad;
        float early_good;
        float early_great;
        float early_pgreat;
        float late_pgreat;
        float late_great;
        float late_good;
        float late_bad;
    };

    struct player_timing_t
    {
        timing_t keys;
        timing_t scratch;
    };

    struct timing_data_t
    {
        char pad_0000[292]; //0x0000
        player_timing_t timing[2]; //0x0124
    };

	// game module
	extern std::uint8_t* base;
	extern std::optional<offsets> addr;

	// global variables
	extern CApplicationConfig* config;
	extern state_t* state;
	extern bool* in_gameplay;
	extern random_data_t* random_data;
	extern player_scores_t* scores[2];
	extern rival_scores_t* rival_scores[2];
    extern play_field_t* play_field;
	extern std::unordered_map<std::uint32_t, music_entry_t*> music_map;

	// state setup
	auto resolve() -> bool;
	auto set_soft_rev(std::uint8_t rev) -> void;
}