versions.push_back({
    .GAME_VERSION             = "2025082500-010",

    .DLL_CODE_SIZE            = 0x0c92600,
    .DLL_ENTRYPOINT           = 0x0b1c7bc,
    .DLL_IMAGE_SIZE           = 0xb8b2000,

    .SOFT_REV_PATCH           = base + 0x0967079, // call replaced by `mov byte ptr [rdi+5], rev`
    .INPUT_POLL_FN            = base + 0x0a84840, // IO::InputManagerIIDX vftable slot 3
    .ARENA_PHASE_PATCH        = base + 0x05943d0, // callee of the arena phase clamp
    .MUSIC_SELECT_CTOR        = base + 0x05f4830, // builds the mlist_sel/thumbnail textures
    .DAN_SELECT_CTOR          = base + 0x08d9980, // CDanSelectScene vftable slot 13 (scene enter)
    .SCENE_DTOR               = base + 0x0982760, // prologue pattern [pattern]
    .STAGE_RESULT_FN          = base + 0x0945f90, // CStageResultScene vftable slot 13 (scene enter)
    .RESET_STATE_FN           = base + 0x05a0bc0, // clears one player's game data block
    .MSELECT_GENRE_C          = base + 0x05f36c0, // renders the hovered entry's genre and title
    .MSELECT_GENRE_B          = base + 0x05f3751, // cmp music_entry::texture_genre, 0 (7 bytes, patched to jmp)
    .MSELECT_GENRE_A          = base + 0x05f382d, // call to the wide genre text renderer
    .MSELECT_GENRE_TEXT_FN    = base + 0x05e11c0, // callee of MSELECT_GENRE_A
    .INIT_TEXT_RENDER_FN      = base + 0x05e0250, // debug text renderer used for the boot progress line
    .BOOT_MODEL_TEXT_CALL     = base + 0x0957e2b, // call that draws the boot model string
    .TITLE_MODEL_TEXT_CALL    = base + 0x094f3ec, // call that draws the title-screen model string
    .TEXT_INIT_FN             = base + 0x0336b90, // initialises the text property block (first callee)
    .TEXT_RENDER_FN           = base + 0x05e05d0, // public wrapper around the ASCII text draw routine
    .RESULT_ARTIST_FN         = base + 0x0939439, // call that draws the result artist text
    .LOAD_CHART_FN_B          = base + 0x0833d50, // reads one .1 chart into the scratch buffer
    .LOAD_CHART_FN_A          = base + 0x0833e70, // sole caller of LOAD_CHART_FN_B
    .REMAP_INDEX_FN           = base + 0x0833ec0, // maps the in-game difficulty index onto the .1 chart index
    .CHART_ANALYZE_FN         = base + 0x0832cc0, // boot-time chart load + analyze pass
    .CHART_ANALYZE_RESULT     = base + 0x0832ea1, // instruction after the chart fread (rax = bytes read, rdi = buffer)
    .CHART_CALC_RADAR_FN      = base + 0x0833700, // prologue pattern [pattern]
    .GET_APP_CONFIG           = base + 0x0954370, // returns CApplicationConfig
    .GET_MUSIC_DATA           = base + 0x096a540, // returns the music_data.bin blob
    .LOAD_AUDIO_FN            = base + 0x0a9a2f0, // prologue pattern [pattern]
    .GET_SOUND_ENTRY_FN       = base + 0x0a99e90, // prologue pattern [pattern]
    .XRPC_APPLY_FN            = base + 0x0aba6e0, // the game's wrapper around avs2-ea3 xrpc_apply
    .REG_DISPATCH_FN          = base + 0x0933710, // builds and fires the music.reg request
    .REG_PATCH_ADDR           = base + 0x093448e, // call that actually queues the score save request
    .SCORE_INVALID_FN         = base + 0x083a660, // shared by the result scene and the score save request
    .DAN_SAVE_FN              = base + 0x05a8870, // prologue pattern [pattern]
    .EAAPPLI_SAVE_FN          = base + 0x05acab0, // prologue pattern [pattern]
    .MDATA_LOAD_FN            = base + 0x094cc70, // CMonitorCheckScene vftable slot 15 (runs after music_data is loaded)
    .IS_BTN_DOWN_FN_A         = base + 0x082b9cd, // cmp autoplay flag inside the `button pressed` helper
    .IS_BTN_DOWN_FN_B         = base + 0x082ba6d, // cmp autoplay flag inside the `button held` helper
    .AUTO_BEAM_PATCH          = base + 0x0924f9b, // cmp autoplay flag in the lane beam renderer
    .AUTO_BEAM_FN             = base + 0x0924d40, // lane beam renderer; rcx -> per-player block whose first dword is the player index
    .CARD_OUT_VFUNC           = base + 0x0d70a30, // CCardOutScene vftable slot 0
    .RENDERER_PATCH           = base + 0x05b8d12, // call nop'd to freeze rendering while the vtable is swapped
    .APPLY_RANDOM_FN          = base + 0x08301c0, // prologue pattern [pattern]
    .DARK_MODE_PATCH          = base + 0x09218ee, // `je` over the play field draw, patched to an unconditional jmp
    .MEASURE_PATCH            = base + 0x0830abe, // branch over the measure bar draw, forced unconditional
    .BPM_BAR_PATCH            = base + 0x0920cb8, // start of the bpm gradient draw, replaced by a jmp past it
    .BPM_BAR_PATCH_JMP        =        0x00000ba, // relative jump to 0x920d77
    .PLAY_FIELD_LOAD          = base + 0x0829520, // fills the play field note table
    .RETRY_CHECK_A            = base + 0x092e0af, // call testing the EFFECT button
    .RETRY_CHECK_B            = base + 0x092e0c1, // call testing the VEFX button
    .GHOST_TARGET_FN          = base + 0x09258a8, // call that renders the '+????' ghost text
    .GRAPH_TARGET_FN          = base + 0x0a4a8b5, // call that renders the 'TARGET:+????' text
    .GRAPH_CONDITION          = base + 0x0a4a86d, // gate on whether the TARGET text is drawn
    .FAIL_ANIMATION_FN        = base + 0x092e8ab, // call that starts the stage failed animation (dl = play it)
    .FAIL_PLAY_SFX_FN         = base + 0x092e8da, // call that plays the stage failed sound effect (rcx = sound id)
    .FAIL_DURATION_JMP        = base + 0x092e270, // `jl` deciding whether the failed animation is still running
    .WNDPROC_FN               = base + 0x0955850, // PropClassWindowProc
    .TIMING_HOOK_FN           = base + 0x082e340, // jump-table dispatcher that installs the judge windows
    .ATTRACT_SELECT_FN        = base + 0x08b1610, // picks the next attract demo chart

    // offsets: data
    .GAME_MODEL               = base + 0x1076270, // the mutable copy of the ea3 model string
    .GAME_STATE               = base + 0xa9fa900, // state block; p1_active/p2_active at +0x10/+0x14 pin it down
    .INPUT_MANAGER            = base + 0xafbbbb0, // the InputManagerIIDX singleton (its vftable pointer is written here)
    .PLAYER_BLOCK             = base + 0x31b9fe8, // base of player 0's game data block
    .SCORES_P1                = base + 0x31e8f44, // player 0 score table (player block + the score updater's displacement)
    .SCORES_P2                = base + 0x6b3f0ec, // player 1 score table
    .RIVAL_SCORES_P1          = base + 0x3976968, // player 0 rival score table
    .RIVAL_SCORES_P2          = base + 0x72ccb10, // player 1 rival score table
    .MAX_ENTRIES              =        0x00080e8, // music entries per play style in the score tables
    .D3D9_DEVICE              = base + 0xa9fb540, // graphics manager +0xE0 (object at 0xa9fb460; delta assumed) [assumed]
    .AUTO_PLAY                = base + 0xa7b7f0c, // autoplay flag inside the play data block
    .RANDOM_DATA              = base + 0xa512ce0, // object holding the post-random lane order
    .PLAY_STATE               = base + 0xa510bf0, // per-play score/note counters and the pacemaker target
    .PLAY_SESSION             = base + 0xa8ccf10, // gameplay session block (pacemaker type, personal best, in-game flag)
    .DEAD_STATE               = base + 0xa8ccac0 // per-player alive flags
});
