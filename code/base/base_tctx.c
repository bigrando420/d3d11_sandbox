per_thread TCTX *tl_tctx = 0;

engine_function TCTX
MakeTCTX(void)
{
    TCTX tctx = {0};
    for(U64 arena_idx = 0; arena_idx < ArrayCount(tctx.arenas); arena_idx += 1)
    {
        tctx.arenas[arena_idx] = M_ArenaAlloc(Gigabytes(8));
    }
    return tctx;
}

engine_function void
SetTCTX(TCTX *tctx)
{
    tl_tctx = tctx;
}

engine_function TCTX *
GetTCTX(void)
{
    return tl_tctx;
}

engine_function void
RegisterThreadFileAndLine_(char *file, int line)
{
    TCTX *tctx = GetTCTX();
    tctx->file_name = file;
    tctx->line_number = line;
}

engine_function M_Temp
GetScratch(M_Arena **conflicts, U64 conflict_count)
{
    M_Temp scratch = {0};
    TCTX *tctx = GetTCTX();
    for(U64 tctx_idx = 0; tctx_idx < ArrayCount(tctx->arenas); tctx_idx += 1)
    {
        B32 is_conflicting = 0;
        for(M_Arena **conflict = conflicts; conflict < conflicts+conflict_count; conflict += 1)
        {
            if(*conflict == tctx->arenas[tctx_idx])
            {
                is_conflicting = 1;
                break;
            }
        }
        if(is_conflicting == 0)
        {
            scratch.arena = tctx->arenas[tctx_idx];
            scratch.pos = scratch.arena->pos;
            break;
        }
    }
    return scratch;
}
