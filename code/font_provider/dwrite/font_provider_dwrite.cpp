// NOTE(rjf): 1000x thanks to Allen Webster for making his example at
// https://github.com/4th-dimention/examps/tree/master/win32-direct-write.
// This API would've been mind-numbing to crack without it.

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render.h"
#include "font_provider/font_provider.h"

#include "font_provider_dwrite.h"

exported FP_BakedFontInfo
BakedFontInfoFromPath(M_Arena *arena, FP_TextAtomArray atoms, F32 size, F32 dpi, String8 path)
{
    M_Temp scratch = GetScratch(&arena, 1);
    F32 dots_per_inch = 1.f / 72.f;
    String16 path16 = Str16From8(scratch.arena, path);
    COLORREF bg_color = RGB(0, 0, 0);
    COLORREF fg_color = RGB(255, 255, 255);
    HRESULT error = 0;
    
    //- rjf: figure out atlas info
    // TODO(rjf): make this smarter
    Vec2S64 atlas_dim = V2S64(1024, 1024);
    
    //- rjf: make dwrite factory... oh boy...
    IDWriteFactory *dwrite_factory = 0;
    error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite_factory);
    
    //- rjf: make a "font file reference"... oh boy x2...
    IDWriteFontFile *font_file = 0;
    error = dwrite_factory->CreateFontFileReference((WCHAR *)path16.str, 0, &font_file);
    
    //- rjf: make dwrite font face
    IDWriteFontFace *font_face = 0;
    error = dwrite_factory->CreateFontFace(DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    
    //- rjf: make dwrite base rendering params
    IDWriteRenderingParams *base_rendering_params = 0;
    error = dwrite_factory->CreateRenderingParams(&base_rendering_params);
    
    //- rjf: make dwrite rendering params
    IDWriteRenderingParams *rendering_params = 0;
    {
        FLOAT gamma = base_rendering_params->GetGamma();
        FLOAT enhanced_contrast = base_rendering_params->GetEnhancedContrast();
        FLOAT clear_type_level = base_rendering_params->GetClearTypeLevel();
        error = dwrite_factory->CreateCustomRenderingParams(gamma,
                                                            enhanced_contrast,
                                                            clear_type_level,
                                                            DWRITE_PIXEL_GEOMETRY_FLAT,
                                                            //DWRITE_PIXEL_GEOMETRY_RGB,
                                                            DWRITE_RENDERING_MODE_DEFAULT,
                                                            &rendering_params);
    }
    
    //- rjf: setup dwrite/gdi interop
    IDWriteGdiInterop *dwrite_gdi_interop = 0;
    error = dwrite_factory->GetGdiInterop(&dwrite_gdi_interop);
    
    //- rjf: get font metrics
    DWRITE_FONT_METRICS font_metrics = {0};
    font_face->GetMetrics(&font_metrics);
    F32 line_advance_design_units = (F32)font_metrics.ascent + (F32)font_metrics.descent + (F32)font_metrics.lineGap;
    F32 line_advance_em = line_advance_design_units / (F32)font_metrics.designUnitsPerEm;
    F32 line_advance_pixels = line_advance_em * size * dpi * dots_per_inch;
    line_advance_pixels = (F32)(S32)(line_advance_pixels + 1.f);
    
    //- rjf: calculate derived sizes
    F32 pixel_per_em = size*dots_per_inch*dpi;
    F32 pixel_per_design_unit = pixel_per_em/((F32)font_metrics.designUnitsPerEm);
    
    //- rjf: make dwrite bitmap for rendering
    IDWriteBitmapRenderTarget *render_target = 0;
    error = dwrite_gdi_interop->CreateBitmapRenderTarget(0, atlas_dim.x, atlas_dim.y, &render_target);
    
    //- rjf: get bitmap & clear
    HDC dc = render_target->GetMemoryDC();
    {
        HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
        SetDCPenColor(dc, bg_color);
        SelectObject(dc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(dc, bg_color);
        Rectangle(dc, 0, 0, atlas_dim.x, atlas_dim.y);
        SelectObject(dc, original);
    }
    
    //- rjf: set up block for all glyph arrays
    U64 glyph_array_count = atoms.count;
    FP_BakedGlyphArray *glyph_arrays = PushArrayZero(arena, FP_BakedGlyphArray, glyph_array_count);
    
    //- rjf: render glyphs & fill glyph arrays
    Vec2F32 draw_p = {4, (F32)atlas_dim.y-4};
    Vec2F32 initial_draw_p = draw_p;
    F32 line_advance_height = line_advance_pixels;
    for(U64 atom_idx = 0; atom_idx < atoms.count; atom_idx += 1)
    {
        FP_TextAtom *atom = atoms.v + atom_idx;
        
        // TODO(rjf): right now I am not handling complex multi-codepoint maps,
        // and am instead just supporting contiguous ranges of codepoints. This
        // is to let me support English etc. easily, but I wanted to get the
        // types, and shape of the API, somewhat close to being right before
        // moving on.
        
        // rjf: allocate glyph storage
        glyph_arrays[atom_idx].count = Dim1U64(atom->cp_range);
        glyph_arrays[atom_idx].v = PushArrayZero(arena, FP_BakedGlyph, glyph_arrays[atom_idx].count);
        
        // rjf: fill glyphs
        for(U64 codepoint64 = atom->cp_range.min, next_idx = 0;
            codepoint64 < atom->cp_range.max;
            codepoint64 = next_idx)
        {
            next_idx = codepoint64 + 1;
            
            U32 codepoint = (U32)codepoint64;
            U16 codepoint_idx = 0;
            error = font_face->GetGlyphIndices(&codepoint, 1, &codepoint_idx);
            
            // rjf: draw glyph
            DWRITE_GLYPH_RUN glyph_run = {0};
            {
                glyph_run.fontFace = font_face;
                glyph_run.fontEmSize = size*dpi*dots_per_inch;
                glyph_run.glyphCount = 1;
                glyph_run.glyphIndices = &codepoint_idx;
            }
            RECT bounding_box = {0};
            error = render_target->DrawGlyphRun(draw_p.x, draw_p.y,
                                                DWRITE_MEASURING_MODE_NATURAL,
                                                &glyph_run,
                                                rendering_params,
                                                fg_color,
                                                &bounding_box);
            
            // rjf: get glyph storage
            FP_BakedGlyph *glyph = 0;
            {
                glyph = glyph_arrays[atom_idx].v + codepoint64;
            }
            
            // rjf: store glyph info
            if(glyph != 0)
            {
                DWRITE_GLYPH_METRICS glyph_metrics = {0};
                error = font_face->GetDesignGlyphMetrics(&codepoint_idx, 1, &glyph_metrics, 0);
                glyph->src.x0  = (F32)bounding_box.left;
                glyph->src.y0  = (F32)bounding_box.top - 1.f;
                glyph->src.x1  = (F32)bounding_box.right;
                glyph->src.y1  = (F32)bounding_box.bottom + 1.f;
                glyph->off.x   = (F32)(bounding_box.left - draw_p.x);
                glyph->off.y   = (F32)(bounding_box.top - draw_p.y) - 4.f;
                glyph->size.x  = (F32)(bounding_box.right - bounding_box.left);
                glyph->size.y  = (F32)(bounding_box.bottom - bounding_box.top) + 2.f;
                F32 advance = (F32)glyph_metrics.advanceWidth * pixel_per_design_unit;
                if((S32)advance < advance)
                {
                    advance = (S32)advance + 1;
                }
                glyph->advance = advance;
            }
            
            // rjf: advance draw position
            if(glyph != 0)
            {
                line_advance_height = Max(line_advance_pixels, (F32)(bounding_box.bottom - bounding_box.top + 4));
                draw_p.x += Dim2F32(glyph->src).x + 4;
                if(draw_p.x >= (F32)atlas_dim.x)
                {
                    draw_p.x = initial_draw_p.x;
                    draw_p.y -= line_advance_height + 1;
                    line_advance_height = line_advance_pixels;
                    next_idx = codepoint64;
                }
            }
        }
        
    }
    
    //- rjf: get bitmap
    HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
    DIBSECTION dib = {0};
    GetObject(bitmap, sizeof(dib), &dib);
    
    //- rjf: build result
    FP_BakedFontInfo result = {0};
    {
        result.atlas_dim = atlas_dim;
        
        // rjf: build atlas
        result.atlas_data = PushArrayZero(arena, U8, atlas_dim.x*atlas_dim.y*4);
        {
            U8 *in_data   = (U8 *)dib.dsBm.bmBits;
            U64 in_pitch  = (U64)dib.dsBm.bmWidthBytes;
            U8 *out_data  = (U8 *)result.atlas_data;
            U64 out_pitch = atlas_dim.x * 4;
            
            U8 *in_line = (U8 *)in_data;
            U8 *out_line = out_data;
            for(U64 y = 0; y < atlas_dim.y; y += 1)
            {
                U8 *in_pixel = in_line;
                U8 *out_pixel = out_line;
                for(U64 x = 0; x < atlas_dim.x; x += 1)
                {
                    out_pixel[0] = 255;//in_pixel[0];
                    out_pixel[1] = 255;//in_pixel[1];
                    out_pixel[2] = 255;//in_pixel[2];
                    out_pixel[3] = in_pixel[0];
                    in_pixel += 4;
                    out_pixel += 4;
                }
                in_line += in_pitch;
                out_line += out_pitch;
            }
        }
        
        // rjf: fill direct map
        result.glyph_array_count = glyph_array_count;
        result.glyph_arrays = glyph_arrays;
        
        // rjf: fill metrics
        result.line_advance = line_advance_pixels;
    }
    
    ReleaseScratch(scratch);
    return result;
}
