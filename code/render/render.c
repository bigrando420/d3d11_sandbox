#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

////////////////////////////////
//~ rjf: Helpers

engine_function R_Glyph
R_GlyphFromFontCodepoint(R_Font font, U32 codepoint)
{
    R_Glyph glyph = {0};
    if(font.direct_map_first <= codepoint && codepoint < font.direct_map_opl)
    {
        glyph = font.direct_map[codepoint - font.direct_map_first];
    }
    // TODO(rjf): look up the glyph in the indirect mapping table
    return glyph;
}

engine_function Vec2F32
R_AdvanceFromString(R_Font font, String8 string)
{
    Vec2F32 v = {0};
    for(U64 offset = 0; offset<string.size;)
    {
        DecodedCodepoint decode = DecodeCodepointFromUtf8(string.str+offset, string.size-offset);
        U32 codepoint = decode.codepoint;
        if(decode.advance == 0)
        {
            break;
        }
        offset += decode.advance;
        
        R_Glyph glyph = R_GlyphFromFontCodepoint(font, codepoint);
        v.x += glyph.advance;
    }
    v.y += font.line_advance;
    return v;
}

engine_function String8
R_PushRectData(M_Arena *arena, Rng2F32 dst, Rng2F32 src, Vec4F32 color00, Vec4F32 color01, Vec4F32 color10, Vec4F32 color11, F32 corner_radius, F32 border_thickness)
{
    F32 *data = PushArrayZero(arena, F32, R_Rect_FloatsPerInstance);
    F32 *write = data;
    *write++ = dst.x0;
    *write++ = dst.y0;
    *write++ = dst.x1;
    *write++ = dst.y1;
    *write++ = src.x0;
    *write++ = src.y0;
    *write++ = src.x1;
    *write++ = src.y1;
    *write++ = color00.x;        // color00_1
    *write++ = color00.y;        // color00_2
    *write++ = color00.z;        // color00_3
    *write++ = color00.w;        // color00_4
    *write++ = color01.x;        // color01_1
    *write++ = color01.y;        // color01_2
    *write++ = color01.z;        // color01_3
    *write++ = color01.w;        // color01_4
    *write++ = color10.x;        // color10_1
    *write++ = color10.y;        // color10_2
    *write++ = color10.z;        // color10_3
    *write++ = color10.w;        // color10_4
    *write++ = color11.x;        // color11_1
    *write++ = color11.y;        // color11_2
    *write++ = color11.z;        // color11_3
    *write++ = color11.w;        // color11_4
    *write++ = corner_radius;    // corner radius
    *write++ = border_thickness; // border thickness
    return Str8((U8 *)data, sizeof(F32)*R_Rect_FloatsPerInstance);
}

engine_function String8
R_PushTriangleData(M_Arena *arena, Vec2F32 p0, Vec4F32 c0, Vec2F32 p1, Vec4F32 c1, Vec2F32 p2, Vec4F32 c2)
{
    F32 rect_data[R_Triangle_FloatsPerInstance] =
    {
        p0.x, p0.y,
        p1.x, p1.y,
        p2.x, p2.y,
        c0.x, c0.y, c0.z, c0.w,
        c1.x, c1.y, c1.z, c1.w,
        c2.x, c2.y, c2.z, c2.w,
    };
    String8 data = Str8((U8 *)rect_data, sizeof(rect_data));
    // TODO(rjf): DON'T USE PUSHSTR8COPY YOU FUCKING IDIOT. it inserts a null
    // terminator which throws all of the command data off.
    return PushStr8Copy(arena, data);
}

engine_function String8
R_PushSetClipData(M_Arena *arena, Rng2F32 clip)
{
    String8 data = Str8((U8 *)&clip, sizeof(clip));
    // TODO(rjf): DON'T USE PUSHSTR8COPY YOU FUCKING IDIOT. it inserts a null
    // terminator which throws all of the command data off.
    return PushStr8Copy(arena, data);
}

engine_function void
R_CmdCopy(R_Cmd *dst, R_Cmd *src)
{
    MemoryCopy(dst, src, sizeof(*dst));
}

engine_function R_CmdNode *
R_CmdListPush(M_Arena *arena, R_CmdList *list, R_Cmd *cmd)
{
    R_CmdNode *n = PushArrayZero(arena, R_CmdNode, 1);
    n->cmd = *cmd;
    QueuePush(list->first, list->last, n);
    list->count += 1;
    return n;
}

engine_function void
R_CmdListJoin(R_CmdList *list, R_CmdList *to_push)
{
    if(list->last && to_push->first)
    {
        list->last->next = to_push->first;
        list->last = to_push->last;
        list->count += to_push->count;
    }
    else
    {
        *list = *to_push;
    }
    MemoryZeroStruct(to_push);
}

engine_function U64
R_BytesPerPixelFromTexture2DFormat(R_Texture2DFormat fmt)
{
    U64 result = 0;
    switch(fmt)
    {
        default:
        case R_Texture2DFormat_R8:    result = 1; break;
        case R_Texture2DFormat_RGBA8: result = 4; break;
    }
    return result;
}

engine_function R_Backend
R_BackendLoad(String8 path)
{
    R_Backend backend = {0};
    backend.library = OS_LibraryOpen(path);
    backend.EquipOS          = (R_EquipOSFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("EquipOS"));
    backend.EquipWindow      = (R_EquipWindowFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("EquipWindow"));
    backend.UnequipWindow    = (R_UnequipWindowFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("UnequipWindow"));
    backend.ReserveTexture2D = (R_ReserveTexture2DFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("ReserveTexture2D"));
    backend.ReleaseTexture2D = (R_ReleaseTexture2DFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("ReleaseTexture2D"));
    backend.FillTexture2D    = (R_FillTexture2DFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("FillTexture2D"));
    backend.SizeFromTexture2D= (R_SizeFromTexture2DFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("SizeFromTexture2D"));
    backend.Start            = (R_StartFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("Start"));
    backend.Submit           = (R_SubmitFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("Submit"));
    backend.Finish           = (R_FinishFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("Finish"));
    return backend;
}

engine_function void
R_BackendUnload(R_Backend backend)
{
    OS_LibraryClose(backend.library);
}

engine_function R_Handle
R_HandleZero(void)
{
    R_Handle result = {0};
    return result;
}

engine_function B32
R_HandleMatch(R_Handle a, R_Handle b)
{
    return (a.u64[0] == b.u64[0] &&
            a.u64[1] == b.u64[1] &&
            a.u64[2] == b.u64[2] &&
            a.u64[3] == b.u64[3]);
}

engine_function B32
R_HandleIsNull(R_Handle handle)
{
    return R_HandleMatch(handle, R_HandleZero());
}

engine_function R_Handle
R_LoadTexture(R_Backend backend, R_Handle os_equip, String8 path)
{
    M_Temp scratch = GetScratch(0, 0);
    String8 data = OS_LoadEntireFile(scratch.arena, path).string;
    int width = 0;
    int height = 0;
    int components = 0;
    U8 *decoded_data = stbi_load_from_memory(data.str, data.size, &width, &height, &components, 4);
    R_Handle result = backend.ReserveTexture2D(os_equip, V2S64(width, height), R_Texture2DFormat_RGBA8);
    backend.FillTexture2D(os_equip, result, R2S64(V2S64(0, 0), V2S64(width, height)), Str8(decoded_data, width*height*4));
    stbi_image_free(decoded_data);
    ReleaseScratch(scratch);
    return result;
}

#if 0
engine_function R_Font
R_LoadFont(R_Backend backend, R_Handle os_equip, F32 size, String8 path)
{
    // rjf: pre-requisites
    M_Temp scratch = GetScratch(0, 0);
    
    // rjf: constants
    U32 direct_map_first = 32;
    U32 direct_map_opl = 128;
    Vec2F32 oversample = { 1.f, 1.f };
    Vec2S64 atlas_size = V2S64(1024, 1024);
    
    // rjf: load file
    String8 font_data = OS_LoadEntireFile(scratch.arena, path).string;
    
    // rjf: allocate
    M_Arena *arena = M_ArenaAlloc(Megabytes(256));
    U8 *pixels = PushArrayZero(arena, U8, atlas_size.x*atlas_size.y);
    
    // rjf: calculate basic metrics
    F32 ascent = 0;
    F32 descent = 0;
    F32 line_gap = 0;
    stbtt_GetScaledFontVMetrics(font_data.str, 0, size, &ascent, &descent, &line_gap);
    F32 line_advance = ascent - descent + line_gap;
    
    // rjf: pack
    stbtt_pack_context ctx = {0};
    stbtt_PackBegin(&ctx, pixels, atlas_size.x, atlas_size.y, 0, 2, 0);
    stbtt_PackSetOversampling(&ctx, oversample.x, oversample.y);
    stbtt_packedchar *chardata_for_range = PushArrayZero(scratch.arena, stbtt_packedchar, direct_map_opl-direct_map_first);
    stbtt_pack_range rng =
    {
        size,
        (int)direct_map_first,
        0,
        (int)(direct_map_opl - direct_map_first),
        chardata_for_range,
    };
    stbtt_PackFontRanges(&ctx, font_data.str, 0, &rng, 1);
    stbtt_PackEnd(&ctx);
    
    // rjf: build direct map
    R_Glyph *direct_map = PushArrayZero(arena, R_Glyph, direct_map_opl-direct_map_first);
    for(U32 codepoint = direct_map_first; codepoint < direct_map_opl; codepoint += 1)
    {
        U32 index = codepoint - direct_map_first;
        F32 x_offset = 0;
        F32 y_offset = 0;
        stbtt_aligned_quad quad = {0};
        stbtt_GetPackedQuad(rng.chardata_for_range, atlas_size.x, atlas_size.y, index, &x_offset, &y_offset, &quad, 1);
        R_Glyph *glyph = direct_map + index;
        glyph->src = R2F32(V2F32(quad.s0 * atlas_size.x,
                                 quad.t0 * atlas_size.y),
                           V2F32(quad.s1 * atlas_size.x,
                                 quad.t1 * atlas_size.y));
        glyph->offset = V2F32(quad.x0, quad.y0);
        glyph->advance = x_offset;
    }
    
    // rjf: fill + return
    R_Font font = {0};
    font.arena = arena;
    font.direct_map_first = direct_map_first;
    font.direct_map_opl = direct_map_opl;
    font.direct_map = direct_map;
    font.texture = backend.ReserveTexture2D(os_equip, atlas_size, R_Texture2DFormat_R8);
    font.line_advance = line_advance;
    font.ascent = ascent;
    font.descent = descent;
    backend.FillTexture2D(os_equip, font.texture, R2S64(V2S64(0, 0), atlas_size), Str8(pixels, atlas_size.x*atlas_size.y));
    ReleaseScratch(scratch);
    return font;
}
#endif
