////////////////////////////////
//~ rjf: Globals

#if ENGINE
DR_State *dr_state = 0;
#endif

////////////////////////////////
//~ rjf: Functions

engine_function void
DR_Init(R_Backend backend, R_Handle os_eqp)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(1));
    dr_state = PushArrayZero(arena, DR_State, 1);
    dr_state->arena = arena;
    dr_state->frame_cmd_arena = M_ArenaAlloc(Gigabytes(2));
    dr_state->frame_data_arena = M_ArenaAlloc(Gigabytes(8));
    M_ArenaSetAutoAlign(dr_state->frame_data_arena, 1);
    dr_state->backend = backend;
    dr_state->os_eqp = os_eqp;
    dr_state->white_texture = backend.ReserveTexture2D(os_eqp, V2S64(1, 1), R_Texture2DFormat_RGBA8);
    U8 white[] = { 0xff, 0xff, 0xff, 0xff };
    backend.FillTexture2D(os_eqp, dr_state->white_texture, R2S64(V2S64(0, 0), V2S64(1, 1)), Str8(white, sizeof(white)));
}

engine_function void
DR_BeginFrame(void)
{
    M_ArenaClear(dr_state->frame_cmd_arena);
    M_ArenaClear(dr_state->frame_data_arena);
}

engine_function void
DR_EndFrame(void)
{
}

engine_function R_Cmd *
DR_PushCmd_KCA(DR_Bucket *bucket, R_CmdKind kind, String8 cpu_data, R_Handle albedo_texture)
{
    R_Cmd *cmd = 0;
    R_CmdNode *last_cmd = bucket->cmds.last;
    if(last_cmd != 0 &&
       last_cmd->cmd.kind == kind &&
       cpu_data.str == last_cmd->cmd.cpu_data.str + last_cmd->cmd.cpu_data.size &&
       R_HandleMatch(last_cmd->cmd.albedo_texture, albedo_texture))
    {
        last_cmd->cmd.cpu_data.size += cpu_data.size;
        cmd = &last_cmd->cmd;
    }
    else
    {
        R_Cmd c = {0};
        c.kind = kind;
        c.cpu_data = cpu_data;
        c.albedo_texture = albedo_texture;
        R_CmdNode *n = R_CmdListPush(dr_state->frame_cmd_arena, &bucket->cmds, &c);
        cmd = &n->cmd;
    }
    return cmd;
}

engine_function void
DR_Rect_VCB(DR_Bucket *bucket, Rng2F32 rect, Vec4F32 color00, Vec4F32 color01, Vec4F32 color10, Vec4F32 color11, F32 corner_radius, F32 border_thickness)
{
    String8 cpu_data = R_PushRectData(dr_state->frame_data_arena, rect, R2F32(V2F32(0, 0), V2F32(1, 1)), color00, color01, color10, color11, corner_radius, border_thickness);
    DR_PushCmd_KCA(bucket, R_CmdKind_Rects, cpu_data, dr_state->white_texture);
}

engine_function void
DR_Rect_CB(DR_Bucket *bucket, Rng2F32 rect, Vec4F32 color, F32 corner_radius, F32 border_thickness)
{
    DR_Rect_VCB(bucket, rect, color, color, color, color, corner_radius, border_thickness);
}

engine_function void
DR_Rect_C(DR_Bucket *bucket, Rng2F32 rect, Vec4F32 color, F32 corner_radius)
{
    DR_Rect_VCB(bucket, rect, color, color, color, color, corner_radius, 0.f);
}

engine_function void
DR_Rect_B(DR_Bucket *bucket, Rng2F32 rect, Vec4F32 color, F32 border_thickness)
{
    DR_Rect_VCB(bucket, rect, color, color, color, color, 0.f, border_thickness);
}

engine_function void
DR_Rect(DR_Bucket *bucket, Rng2F32 rect, Vec4F32 color)
{
    DR_Rect_VCB(bucket, rect, color, color, color, color, 0.f, 0.f);
}

engine_function void
DR_Sprite(DR_Bucket *bucket, Vec4F32 color, Rng2F32 rect, Rng2F32 src_rect, R_Handle texture)
{
    String8 cpu_data = R_PushRectData(dr_state->frame_data_arena, rect, src_rect, color, color, color, color, 0.f, 0.f);
    DR_PushCmd_KCA(bucket, R_CmdKind_Rects, cpu_data, texture);
}

engine_function void
DR_Text(DR_Bucket *bucket, Vec4F32 color, Vec2F32 p, R_Font font, String8 text)
{
    String8 cpu_data = {0};
    Vec2F32 glyph_p = p;
    glyph_p.y += font.line_advance;
    for(U64 offset = 0; offset<text.size;)
    {
        DecodedCodepoint decode = DecodeCodepointFromUtf8(text.str+offset, text.size-offset);
        U32 codepoint = decode.codepoint;
        if(decode.advance == 0)
        {
            break;
        }
        offset += decode.advance;
        
        R_Glyph glyph = R_GlyphFromFontCodepoint(font, codepoint);
        Vec2F32 glyph_size = glyph.size;
        Rng2F32 rect = R2F32(V2F32(glyph_p.x + glyph.offset.x,
                                   glyph_p.y + glyph.offset.y),
                             V2F32(glyph_p.x + glyph.offset.x + glyph_size.x,
                                   glyph_p.y + glyph.offset.y + glyph_size.y));
        String8 glyph_data = R_PushRectData(dr_state->frame_data_arena, rect, glyph.src, color, color, color, color, 0.f, 0);
        if(cpu_data.str == 0)
        {
            cpu_data.str = glyph_data.str;
        }
        cpu_data.size += glyph_data.size;
        glyph_p.x += glyph.advance;
    }
    DR_PushCmd_KCA(bucket, R_CmdKind_Rects, cpu_data, font.texture);
}

engine_function void
DR_TextF(DR_Bucket *bucket, Vec4F32 color, Vec2F32 p, R_Font font, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 text = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    DR_Text(bucket, color, p, font, text);
    ReleaseScratch(scratch);
}

engine_function void
DR_CmdList(DR_Bucket *bucket, R_CmdList cmds)
{
    for(R_CmdNode *n = cmds.first; n != 0; n = n->next)
    {
        R_Cmd cmd_copy = {0};
        R_CmdCopy(&cmd_copy, &n->cmd);
        R_CmdListPush(dr_state->frame_cmd_arena, &bucket->cmds, &cmd_copy);
    }
}

engine_function void
DR_Submit(R_Handle window_eqp, R_CmdList cmds)
{
    dr_state->backend.Submit(dr_state->os_eqp, window_eqp, cmds);
}
