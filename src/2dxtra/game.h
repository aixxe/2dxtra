#pragma once

#include <cstddef>
#include <cstring>
#include <optional>
#include <offsets.h>
#include <unordered_map>

namespace bm2dx
{
	// enums
	enum class play_style: int { SP = 0, DP = 1 };
	enum class button: std::uint8_t { EFFECT = 18 };

	// constants
	auto constexpr MAX_RIVALS = 6;
	auto constexpr CHART_EVENT_CAPACITY = 0x3000;
	auto constexpr MAX_PLAY_NOTES = 16000;
	auto constexpr SCRATCH_COLUMN = 7;
	auto constexpr BGM_COLUMN = 8;
	auto constexpr PLAYABLE_COLUMNS = 8;

	auto constexpr REQUEST_LOBBY_ENTRY = "lobby.entry";
	auto constexpr REQUEST_LOBBY_UPDATE = "lobby.update";
	auto constexpr REQUEST_LOBBY_DELETE = "lobby.delete";
	auto constexpr REQUEST_GAME_SYSTEM_INFO = "gameSystem.systemInfo";

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

    // event types in the .1 chart file format
    enum class chart_event_type: std::uint8_t
    {
        NOTE_P1 = 0x00,
        NOTE_P2 = 0x01,
        SAMPLE_P1 = 0x02,
        SAMPLE_P2 = 0x03,
        TEMPO = 0x04,
        METER = 0x05,
        END_OF_SONG = 0x06,
        BGM = 0x07,
        TIMING_WINDOW = 0x08,
        MEASURE_BAR = 0x0C,
        NOTE_COUNT = 0x10,
    };

    struct chart_event_t
	{
		std::int32_t offset = 0; //0x0000
		chart_event_type type = chart_event_type::NOTE_P1; //0x0004
		std::int8_t parameter = 0; //0x0005
		std::int16_t value = 0; //0x0006
	}; static_assert(sizeof(chart_event_t) == 0x8);

	auto constexpr CHART_BUFFER_BYTES = CHART_EVENT_CAPACITY * sizeof(chart_event_t);

    // Read / write a chart event to/from the little-endian file format.
    // The struct layout matches the on-disk record exactly.
    auto inline read_chart_event(const std::uint8_t* p) -> chart_event_t
    {
        chart_event_t e {};
        std::memcpy(&e, p, sizeof(e));
        return e;
    }

    auto inline write_chart_event(std::uint8_t* p, const chart_event_t& e) -> void
		{ std::memcpy(p, &e, sizeof(e)); }

    struct chart_buffer_t
    {
        chart_event_t events[CHART_EVENT_CAPACITY]; //0x0000
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
        auto end() { return notes + MAX_PLAY_NOTES; }

        play_note_t notes[MAX_PLAY_NOTES]; //0x0000
    }; static_assert(sizeof(play_notes_t) == 0x138800);

    struct play_field_t
    {
        play_notes_t notes[2]; //0x0000
    }; static_assert(sizeof(play_field_t) == 0x271000);

    struct text_props_t
    {
        std::uint8_t pad_0000[64]; //0x0000
		std::int32_t spacing; //0x0040
		std::uint8_t pad_0044[16]; //0x0044
		std::int32_t h_align; //0x0054
        std::int32_t v_align; //0x0058
        std::uint8_t pad_005c[252]; //0x005C (extra space for text)
    };
    static_assert(offsetof(text_props_t, h_align) == 0x54);
    static_assert(offsetof(text_props_t, v_align) == 0x58);
    static_assert(sizeof(text_props_t) == 0x158);

	struct random_data_t
	{
		std::uint8_t pad_0000[0x10]; //0x0000
		std::uint32_t columns[2][PLAYABLE_COLUMNS]; //0x0010
	}; static_assert(sizeof(random_data_t) == 0x50);

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
		std::uint32_t buttons; //0x0000
		std::uint32_t buttons_edge; //0x0004
		std::uint8_t pad_0008[0x48]; //0x0008
		std::int32_t p1_turntable; //0x0050
		std::int32_t p1_turntable_delta; //0x0054
		std::int32_t p2_turntable; //0x0058
		std::int32_t p2_turntable_delta; //0x005C
	}; static_assert(sizeof(input_t) == 0x60);

	auto constexpr INPUT_BUTTON_BYTES = offsetof(input_t, pad_0008);
	auto constexpr INPUT_TURNTABLE_BYTES = sizeof(input_t) - offsetof(input_t, p1_turntable);

    struct InputManagerIIDX
	{
		void* vft; //0x0000
		input_t data; //0x0008
	}; static_assert(sizeof(InputManagerIIDX) == 0x68);

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
		/* 0x0000 */ const wchar_t title[128];
        /* 0x0100 */ const char title_ascii[64];
        /* 0x0140 */ const wchar_t genre[64];
        /* 0x01C0 */ const wchar_t artist[128];
        /* 0x02C0 */ const wchar_t license[128];
        /* 0x03C0 */ std::int32_t texture_title;
        /* 0x03C4 */ std::int32_t texture_artist;
        /* 0x03C8 */ std::int32_t texture_genre;
        /* 0x03CC */ std::int32_t texture_load;
        /* 0x03D0 */ std::int32_t texture_list;
        /* 0x03D4 */ std::int32_t texture_license;
        /* 0x03D8 */ std::int32_t font_idx;
        /* 0x03DC */ std::uint16_t game_version;
        /* 0x03DE */ std::uint16_t other_folder;
        /* 0x03E0 */ std::uint16_t bemani_folder;
        /* 0x03E2 */ std::uint16_t recommend_beginner_folder;
        /* 0x03E4 */ std::uint16_t recommend_iidx_folder;
        /* 0x03E6 */ std::uint16_t recommend_bemani_folder;
        /* 0x03E8 */ std::uint16_t splittable_diff;
        /* 0x03EA */ std::uint8_t pad_03EA[2];
        /* 0x03EC */ std::uint8_t spb_rating;
        /* 0x03ED */ std::uint8_t spn_rating;
        /* 0x03EE */ std::uint8_t sph_rating;
        /* 0x03EF */ std::uint8_t spa_rating;
        /* 0x03F0 */ std::uint8_t spl_rating;
        /* 0x03F1 */ std::uint8_t pad_03F1[1];
        /* 0x03F2 */ std::uint8_t dpn_rating;
        /* 0x03F3 */ std::uint8_t dph_rating;
        /* 0x03F4 */ std::uint8_t dpa_rating;
        /* 0x03F5 */ std::uint8_t dpl_rating;
        /* 0x03F6 */ std::uint8_t pad_03F6[6];
        /* 0x03FC */ std::uint32_t spb_bpm_max;
        /* 0x0400 */ std::uint32_t spb_bpm_min;
        /* 0x0404 */ std::uint32_t spn_bpm_max;
        /* 0x0408 */ std::uint32_t spn_bpm_min;
        /* 0x040C */ std::uint32_t sph_bpm_max;
        /* 0x0410 */ std::uint32_t sph_bpm_min;
        /* 0x0414 */ std::uint32_t spa_bpm_max;
        /* 0x0418 */ std::uint32_t spa_bpm_min;
        /* 0x041C */ std::uint32_t spl_bpm_max;
        /* 0x0420 */ std::uint32_t spl_bpm_min;
        /* 0x0424 */ std::uint8_t pad_0424[8];
        /* 0x042C */ std::uint32_t dpn_bpm_max;
        /* 0x0430 */ std::uint32_t dpn_bpm_min;
        /* 0x0434 */ std::uint32_t dph_bpm_max;
        /* 0x0438 */ std::uint32_t dph_bpm_min;
        /* 0x043C */ std::uint32_t dpa_bpm_max;
        /* 0x0440 */ std::uint32_t dpa_bpm_min;
        /* 0x0444 */ std::uint32_t dpl_bpm_max;
        /* 0x0448 */ std::uint32_t dpl_bpm_min;
        /* 0x044C */ std::uint8_t pad_044C[48];
        /* 0x047C */ std::uint32_t spb_note_count;
        /* 0x0480 */ std::uint32_t spn_note_count;
        /* 0x0484 */ std::uint32_t sph_note_count;
        /* 0x0488 */ std::uint32_t spa_note_count;
        /* 0x048C */ std::uint32_t spl_note_count;
        /* 0x0490 */ std::uint8_t pad_0490[4];
        /* 0x0494 */ std::uint32_t dpn_note_count;
        /* 0x0498 */ std::uint32_t dph_note_count;
        /* 0x049C */ std::uint32_t dpa_note_count;
        /* 0x04A0 */ std::uint32_t dpl_note_count;
	    /* 0x04A4 */ std::uint8_t pad_04A4[24];
	    /* 0x04BC */ std::int32_t spb_cn_type;
	    /* 0x04C0 */ std::int32_t spn_cn_type;
	    /* 0x04C4 */ std::int32_t sph_cn_type;
	    /* 0x04C8 */ std::int32_t spa_cn_type;
	    /* 0x04CC */ std::int32_t spl_cn_type;
	    /* 0x04D0 */ std::uint8_t pad_04D0[4];
	    /* 0x04D4 */ std::int32_t dpn_cn_type;
	    /* 0x04D8 */ std::int32_t dph_cn_type;
	    /* 0x04DC */ std::int32_t dpa_cn_type;
	    /* 0x04E0 */ std::int32_t dpl_cn_type;
	    /* 0x04E4 */ std::uint8_t pad_04E4[24];
		/* 0x04FC */ notes_radar_t spb_notes_radar;
		/* 0x0514 */ notes_radar_t spn_notes_radar;
		/* 0x052C */ notes_radar_t sph_notes_radar;
		/* 0x0544 */ notes_radar_t spa_notes_radar;
		/* 0x055C */ notes_radar_t spl_notes_radar;
		/* 0x0574 */ std::uint8_t pad_0574[24];
		/* 0x058C */ notes_radar_t dpn_notes_radar;
		/* 0x05A4 */ notes_radar_t dph_notes_radar;
		/* 0x05BC */ notes_radar_t dpa_notes_radar;
		/* 0x05D4 */ notes_radar_t dpl_notes_radar;
        /* 0x05EC */ char pad_05EC[144];
        /* 0x067C */ std::int32_t id;
        /* 0x0680 */ std::int32_t volume;
        /* 0x0684 */ char pad_0684[372];
    }; static_assert(sizeof(music_entry_t) == 0x7F8);

    struct music_data_t
    {
        /* 0x0000 */ const char magic[4]; // "IIDX"
        /* 0x0004 */ std::int32_t version;
        /* 0x0008 */ std::int16_t entries;
        /* 0x000C */ std::int32_t occupied_entries;
        /* 0x0010 */ std::int32_t indexes[1];
    };
    static_assert(offsetof(music_data_t, occupied_entries) == 0x0C);
    static_assert(offsetof(music_data_t, indexes) == 0x10);

    auto music_first(music_data_t* data) -> music_entry_t*;

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

	struct play_counters_t
	{
		std::int32_t ex_score; //0x0000
		std::uint8_t pad_0004[8]; //0x0004
		std::int32_t note_current; //0x000C
		std::int32_t note_total; //0x0010
		std::uint8_t pad_0014[852]; //0x0014
	}; static_assert(sizeof(play_counters_t) == 0x368);

	struct play_state_t
	{
		std::uint8_t pad_0000[16]; //0x0000
		play_counters_t players[2]; //0x0010
		std::uint8_t pad_06E0[876]; //0x06E0
		std::uint32_t pacemaker_target; //0x0A4C
	}; static_assert(sizeof(play_state_t) == 0xA50);

	struct play_session_t
	{
		std::uint8_t pad_0000[0x54]; //0x0000
		bool in_gameplay; //0x0054
		std::uint8_t pad_0055[0x30F]; //0x0055
		std::uint32_t current_score_pb; //0x0364
		std::uint8_t pad_0368[0x28]; //0x0368
		pacemaker_type pacemaker_type_id; //0x0390
	}; static_assert(offsetof(play_session_t, pacemaker_type_id) == 0x390);

	struct game_score_t
	{
		std::int32_t score[5]; //0x0000
		std::int32_t miss[5]; //0x0014
		std::int8_t clear[5]; //0x0028
		std::int8_t is_populated; //0x002D
		std::uint8_t pad_002E[2]; //0x002E
	}; static_assert(sizeof(game_score_t) == 0x30);

	auto max_entries() -> std::size_t;
	auto player_scores_size() -> std::size_t;
	auto rival_scores_size() -> std::size_t;

	auto player_score(int player, play_style style, std::uint32_t music_id) -> game_score_t*;
	auto rival_score(int player, int rival, play_style style, std::uint32_t music_id) -> game_score_t*;

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
	extern InputManagerIIDX* input_manager;
	extern play_state_t* play_state;
	extern play_session_t* play_session;
	extern dead_state_t* dead_state;
	extern random_data_t* random_data;
	extern game_score_t* scores[2];
	extern game_score_t* rival_scores[2];
    extern play_field_t* play_field;
	extern std::unordered_map<std::uint32_t, music_entry_t*> music_map;

	// state setup
	auto resolve() -> bool;
	auto set_soft_rev(std::uint8_t rev) -> void;
}