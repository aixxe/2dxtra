versions.push_back({
    .GAME_VERSION             = "2026081900-010",

    .DLL_CODE_SIZE            = 0x0c8f800,
    .DLL_ENTRYPOINT           = 0x0b1cccc,
    .DLL_IMAGE_SIZE           = 0xbb95000,

    .SOFT_REV_PATCH           = base + 0x094e6f9, // call replaced by `mov byte ptr [rdi+5], rev`
    .INPUT_POLL_FN            = base + 0x0a7a2f0, // IO::InputManagerIIDX vftable slot 3
    .ARENA_PHASE_PATCH        = base + 0x059e9e0, // callee of the arena phase clamp
    .MUSIC_SELECT_CTOR        = base + 0x0603ee0, // builds the mlist_sel/thumbnail textures
    .DAN_SELECT_CTOR          = base + 0x08cd420, // CDanSelectScene vftable slot 13 (scene enter)
    .SCENE_DTOR               = base + 0x096a220, // prologue pattern [pattern]
    .STAGE_RESULT_FN          = base + 0x092c2d0, // CStageResultScene vftable slot 13 (scene enter)
    .RESET_STATE_FN           = base + 0x05abed0, // clears one player's game data block
    .MSELECT_GENRE_C          = base + 0x0602d20, // renders the hovered entry's genre and title
    .MSELECT_GENRE_B          = base + 0x0602db1, // cmp music_entry::texture_genre, 0 (7 bytes, patched to jmp)
    .MSELECT_GENRE_A          = base + 0x0602e8d, // call to the wide genre text renderer
    .MSELECT_GENRE_TEXT_FN    = base + 0x05ecf80, // callee of MSELECT_GENRE_A
    .INIT_TEXT_RENDER_FN      = base + 0x05ebff0, // debug text renderer used for the boot progress line
    .BOOT_MODEL_TEXT_CALL     = base + 0x093f46b, // call that draws the boot model string
    .TITLE_MODEL_TEXT_CALL    = base + 0x09369dc, // call that draws the title-screen model string
    .TEXT_INIT_FN             = base + 0x0344880, // initialises the text property block (first callee)
    .TEXT_RENDER_FN           = base + 0x05ec370, // public wrapper around the ASCII text draw routine
    .RESULT_ARTIST_FN         = base + 0x091c3e9, // call that draws the result artist text
    .LOAD_CHART_FN_B          = base + 0x0827c60, // reads one .1 chart into the scratch buffer
    .LOAD_CHART_FN_A          = base + 0x0827d80, // sole caller of LOAD_CHART_FN_B
    .CHART_ANALYZE_FN         = base + 0x0826af0, // boot-time chart load + analyze pass
    .CHART_ANALYZE_RESULT     = base + 0x0826d13, // instruction after the chart fread (rax = bytes read, rdi = buffer)
    .CHART_CALC_RADAR_FN      = base + 0x0827610, // prologue pattern [pattern]
    .GET_APP_CONFIG           = base + 0x093b9d0, // returns CApplicationConfig
    .GET_MUSIC_DATA           = base + 0x0951fd0, // returns the music_data.bin blob
    .LOAD_AUDIO_FN            = base + 0x0a8ffa0, // prologue pattern [pattern]
    .GET_SOUND_ENTRY_FN       = base + 0x0a8fb40, // prologue pattern [pattern]
    .XRPC_APPLY_FN            = base + 0x0ab9520, // the game's wrapper around avs2-ea3 xrpc_apply
    .REG_DISPATCH_FN          = base + 0x09164e0, // builds and fires the music.reg request
    .REG_PATCH_ADDR           = base + 0x09173d5, // call that actually queues the score save request
    .SCORE_INVALID_FN         = base + 0x082e510, // shared by the result scene and the score save request
    .DAN_SAVE_FN              = base + 0x05b4270, // prologue pattern [pattern]
    .EAAPPLI_SAVE_FN          = base + 0x05b87a0, // prologue pattern [pattern]
    .MDATA_LOAD_FN            = base + 0x0934260, // CMonitorCheckScene vftable slot 15 (runs after music_data is loaded)
    .IS_BTN_DOWN_FN_A         = base + 0x081edad, // cmp autoplay flag inside the `button pressed` helper
    .IS_BTN_DOWN_FN_B         = base + 0x081ee4d, // cmp autoplay flag inside the `button held` helper
    .AUTO_BEAM_PATCH          = base + 0x0905ebb, // cmp autoplay flag in the lane beam renderer
    .AUTO_BEAM_FN             = base + 0x0905c60, // lane beam renderer; rcx -> per-player block whose first dword is the player index
    .CARD_OUT_VFUNC           = base + 0x0d77780, // CCardOutScene vftable slot 0
    .RENDERER_PATCH           = base + 0x05c6ae2, // call nop'd to freeze rendering while the vtable is swapped
    .APPLY_RANDOM_FN          = base + 0x08236e0, // prologue pattern [pattern]
    .DARK_MODE_PATCH          = base + 0x0902658, // `je` over the play field draw, patched to an unconditional jmp
    .MEASURE_PATCH            = base + 0x0823f29, // branch over the measure bar draw, forced unconditional
    .BPM_BAR_PATCH            = base + 0x09019ca, // start of the bpm gradient draw, replaced by a jmp past it
    .BPM_BAR_PATCH_JMP        =        0x00000ba, // relative jump to 0x901a89
    .PLAY_FIELD_LOAD          = base + 0x081c5c0, // fills the play field note table
    .RETRY_CHECK_A            = base + 0x090fd78, // call testing the EFFECT button
    .RETRY_CHECK_B            = base + 0x090fd8a, // call testing the VEFX button
    .GHOST_TARGET_FN          = base + 0x0906819, // call that renders the '+????' ghost text
    .GRAPH_TARGET_FN          = base + 0x0a3cf15, // call that renders the 'TARGET:+????' text
    .GRAPH_CONDITION          = base + 0x0a3cecd, // gate on whether the TARGET text is drawn
    .FAIL_ANIMATION_FN        = base + 0x091058b, // call that starts the stage failed animation (dl = play it)
    .FAIL_PLAY_SFX_FN         = base + 0x09105ba, // call that plays the stage failed sound effect (rcx = sound id)
    .FAIL_DURATION_JMP        = base + 0x090ff30, // `jl` deciding whether the failed animation is still running
    .WNDPROC_FN               = base + 0x093cea0, // PropClassWindowProc
    .TIMING_HOOK_FN           = base + 0x0821730, // jump-table dispatcher that installs the judge windows
    .ATTRACT_SELECT_FN        = base + 0x08a0ed0, // picks the next attract demo chart

    .IS_SEPARATE_SCRATCH_FN   = base + 0x080a880, // whether separate FAST/SLOW for scratch option is enabled
    .SPRITE_DRAW_FN           = base + 0x034ac50, // named sprite draw helper used for s_fast and s_slow
    .JUDGE_APPLY_FN           = base + 0x08210b0, // applies a note judgment; prologue pattern [pattern]
    .JUDGE_DISPLAY_FN         = base + 0x0909f80, // updates judge, combo and FAST/SLOW state; prologue pattern [pattern]
    .JUDGE_DRAW_FS_KEYS_FN    = base + 0x0909750, // fast/slow renderer sharing the scratch renderer's caller
    .JUDGE_DRAW_FS_SC_FN      = base + 0x0909a40, // renderer referencing s_fast and s_slow
    .JUDGE_DISPLAY_INIT_FN    = base + 0x0909d10, // initializes judgment display state and judge_great_yellow sprites
    .JUDGE_PRESS_RETURN       = base + 0x0821ce1, // return after general timing judgment call; continuation pattern [pattern]
    .JUDGE_RELEASE_RETURN     = base + 0x0820c8a, // return after CN release timing judgment call; continuation pattern [pattern]
    .HI_SPEED_RESET_PATCH     = base + 0x090a253, // xor byte ptr [rsi+28h], 1; native double-START branch
    .HI_SPEED_RESET_CONTINUE  = base + 0x090a36a, // mov rcx, rsi; call lane/green-number predicate
    .HI_SPEED_ADJUST_CALL     = base + 0x090b926, // call 0x90bc70; RCX = controller, 5 bytes
    .GET_PLAYER_OPTIONS       = base + 0x08854a0, // runtime options singleton accessor
    .IS_DOUBLE_PLAY           = base + 0x082e0f0, // SP/DP selector used by option accessors
    .GET_HI_SPEED_MODE        = base + 0x0897aa0, // selected option block +0x1c
    .GET_SAVED_HI_SPEED       = base + 0x0895b80, // option block +4 divided by native speed scale
    .SET_HI_SPEED             = base + 0x0829630, // preserves interpolation, clamps target speed

    // offsets: data
    .GAME_MODEL               = base + 0x1080d40, // the mutable copy of the ea3 model string
    .GAME_STATE               = base + 0xacd79a0, // state block; p1_active/p2_active at +0x10/+0x14 pin it down
    .INPUT_MANAGER            = base + 0xb29a2b0, // the InputManagerIIDX singleton (its vftable pointer is written here)
    .PLAYER_BLOCK             = base + 0x3101488, // base of player 0's game data block
    .SCORES_P1                = base + 0x312f01c, // player 0 score table (player block + the score updater's displacement)
    .SCORES_P2                = base + 0x6c3f3bc, // player 1 score table
    .RIVAL_SCORES_P1          = base + 0x38f73c0, // player 0 rival score table
    .RIVAL_SCORES_P2          = base + 0x7407760, // player 1 rival score table
    .MAX_ENTRIES              =        0x00084d0, // music entries per play style in the score tables
    .D3D9_DEVICE              = base + 0xacd85e0, // IDirect3DDevice9* field assigned after CreateDeviceEx at 0x93c750 (manager +0xe0)
    .AUTO_PLAY                = base + 0xaa9479c, // autoplay flag inside the play data block
    .RANDOM_DATA              = base + 0xa7ef570, // object holding the post-random lane order
    .PLAY_STATE               = base + 0xa7ed450, // per-play score/note counters and the pacemaker target
    .PLAY_SESSION             = base + 0xaba9a30, // gameplay session block (pacemaker type, personal best, in-game flag)
    .DEAD_STATE               = base + 0xaba95e0, // per-player alive flags from the failure handler's shared getter
    .HI_SPEED_STATE           = base + 0xaaad6c0, // singleton returned by 0x8297e0
});
