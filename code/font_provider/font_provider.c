////////////////////////////////
//~ rjf: Helpers

engine_function FP_Backend
FP_BackendLoad(String8 path)
{
    FP_Backend backend = {0};
    backend.library = OS_LibraryOpen(path);
    backend.BakedFontInfoFromPath = (FP_BakedFontInfoFromPathFunction *)OS_LibraryLoadFunction(backend.library, Str8Lit("BakedFontInfoFromPath"));
    return backend;
}

engine_function void
FP_BackendUnload(FP_Backend backend)
{
    OS_LibraryClose(backend.library);
}

engine_function R_Font
FP_FontFromBakedFontInfo(R_Backend backend, R_Handle os_equip, FP_TextAtomArray atoms, FP_BakedFontInfo info)
{
    R_Font font = {0};
    
    // rjf: allocate arena
    font.arena = M_ArenaAlloc(Megabytes(256));
    
    // rjf: fill texture
    font.texture = backend.ReserveTexture2D(os_equip, info.atlas_dim, R_Texture2DFormat_RGBA8);
    backend.FillTexture2D(os_equip, font.texture, R2S64(V2S64(0, 0), info.atlas_dim), Str8(info.atlas_data, info.atlas_dim.x*info.atlas_dim.y*4));
    
    // rjf: fill direct map
    font.direct_map_first = 0;
    font.direct_map_opl = 256;
    font.direct_map = PushArrayZero(font.arena, R_Glyph, font.direct_map_opl-font.direct_map_first);
    for(U64 atom_idx = 0; atom_idx < atoms.count; atom_idx += 1)
    {
        for(U64 codepoint64 = atoms.v[atom_idx].cp_range.min;
            codepoint64 < atoms.v[atom_idx].cp_range.max;
            codepoint64 += 1)
        {
            U32 codepoint = (U32)codepoint64;
            if(font.direct_map_first <= codepoint && codepoint < font.direct_map_opl)
            {
                FP_BakedGlyph *src_glyph = info.glyph_arrays[atom_idx].v + codepoint64 - atoms.v[atom_idx].cp_range.min;
                R_Glyph *dst_glyph = font.direct_map + codepoint - font.direct_map_first;
                dst_glyph->src = src_glyph->src;
                dst_glyph->offset = src_glyph->off;
                dst_glyph->size = src_glyph->size;
                dst_glyph->advance = src_glyph->advance;
            }
        }
    }
    
    // rjf: fill metrics
    font.line_advance = info.line_advance;
    
    return font;
}
