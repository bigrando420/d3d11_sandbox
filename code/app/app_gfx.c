////////////////////////////////
//~ rjf: Globals

#if ENGINE
APP_State *app_state = 0;
read_only APP_View app_g_nil_view =
{
    0,
    APP_ViewOpenStub,
    APP_ViewCloseStub,
    APP_ViewUpdateStub,
};
#endif

////////////////////////////////
//~ rjf: Main API

engine_function void
APP_Init(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(64));
    app_state = PushArrayZero(arena, APP_State, 1);
    app_state->permanent_arena = arena;
    for(U64 idx = 0; idx < ArrayCount(app_state->frame_arenas); idx += 1)
    {
        app_state->frame_arenas[idx] = M_ArenaAlloc(Gigabytes(1));
    }
    app_state->target_fps = OS_DefaultRefreshRate();
    app_state->r_backend = R_BackendLoad(Str8Lit("render_d3d11.dll"));
    app_state->r_os_equip = app_state->r_backend.EquipOS();
    app_state->fp_backend = FP_BackendLoad(Str8Lit("font_provider_dwrite.dll"));
    app_state->default_font = APP_LoadFont(14.f, Str8Lit("Inconsolata-Regular.ttf"));
    app_state->icons_font   = APP_LoadFont(12.f, Str8Lit("icons.ttf"));
}

engine_function B32
APP_Quit(void)
{
    return app_state->first_window == 0;
}

engine_function void
APP_BeginFrame(void)
{
    M_ArenaClear(APP_FrameArena());
    DR_BeginFrame();
    app_state->cursor_kind__last_frame = app_state->cursor_kind;
    app_state->cursor_kind = OS_CursorKind_Null;
}

engine_function void
APP_EndFrame(void)
{
    DR_EndFrame();
    app_state->frame_index += 1;
    if(app_state->cursor_kind__last_frame != app_state->cursor_kind)
    {
        OS_SetCursor(app_state->cursor_kind);
    }
}

////////////////////////////////
//~ rjf: Renderer Wrapper

engine_function R_Handle
APP_LoadTexture(String8 path)
{
    return R_LoadTexture(app_state->r_backend, app_state->r_os_equip, path);
}

engine_function R_Font
APP_LoadFont(F32 size, String8 path)
{
    M_Temp scratch = GetScratch(0, 0);
    Rng1U64List indirect_codepoint_range_list = {0};
    FP_TextAtom atom = {{0}, {0, 256}};
    FP_TextAtomArray atoms = {&atom, 1};
    FP_BakedFontInfo info = app_state->fp_backend.BakedFontInfoFromPath(scratch.arena, atoms, size, 96.f, path);
    R_Font result = FP_FontFromBakedFontInfo(app_state->r_backend, app_state->r_os_equip, atoms, info);
    ReleaseScratch(scratch);
    return result;
}

engine_function Vec2F32
APP_SizeFromTexture(R_Handle handle)
{
    return app_state->r_backend.SizeFromTexture2D(handle);
}

////////////////////////////////
//~ rjf: Views

engine_function APP_View *
APP_NilView(void)
{
    return &app_g_nil_view;
}

engine_function B32
APP_ViewIsNil(APP_View *view)
{
    return view == 0 || view == &app_g_nil_view;
}

engine_function void *
APP_ViewOpenStub(void)
{
    return 0;
}

engine_function void
APP_ViewCloseStub(void *data)
{
}

engine_function void
APP_ViewUpdateStub(OS_EventList *events, Rng2F32 rect, void *data)
{
}

////////////////////////////////
//~ rjf: Frame Info

engine_function U64
APP_FrameIndex(void)
{
    return app_state->frame_index;
}

engine_function F32
APP_DT(void)
{
    return 1.f / app_state->target_fps;
}

engine_function void
APP_Cursor(OS_CursorKind cursor)
{
    app_state->cursor_kind = cursor;
}

////////////////////////////////
//~ rjf: Animation Helpers

engine_function void
APP_AnimateF32_Exp(F32 *x, F32 target, F32 rate)
{
    F32 r = rate * APP_DT();
    r = ClampTop(r, 1.f);
    *x += (target - *x) * r;
}

////////////////////////////////
//~ rjf: Resources

engine_function R_Font
APP_DefaultFont(void)
{
    return app_state->default_font;
}

engine_function R_Font
APP_IconsFont(void)
{
    return app_state->icons_font;
}

////////////////////////////////
//~ rjf: Arenas

engine_function M_Arena *
APP_PermanentArena(void)
{
    return app_state->permanent_arena;
}

engine_function M_Arena *
APP_FrameArena(void)
{
    return app_state->frame_arenas[app_state->frame_index % ArrayCount(app_state->frame_arenas)];
}

////////////////////////////////
//~ rjf: Windows

engine_function APP_Window *
APP_WindowOpen(String8 title, Vec2S64 size, APP_WindowUserHooks hooks)
{
    // rjf: alloc
    APP_Window *window = app_state->free_window;
    if(window == 0)
    {
        window = PushArrayZero(app_state->permanent_arena, APP_Window, 1);
    }
    else
    {
        StackPop(app_state->free_window);
    }
    
    // rjf: fill
    MemoryZeroStruct(window);
    DLLPushBack(app_state->first_window, app_state->last_window, window);
    window->handle = OS_WindowOpen(title, size);
    window->window_equip = app_state->r_backend.EquipWindow(app_state->r_os_equip, window->handle);
    window->user.hooks = hooks;
    return window;
}

engine_function void
APP_WindowClose(APP_Window *window)
{
    app_state->r_backend.UnequipWindow(app_state->r_os_equip, window->window_equip);
    OS_WindowClose(window->handle);
    DLLRemove(app_state->first_window, app_state->last_window, window);
    StackPush(app_state->free_window, window);
}

engine_function APP_Window *
APP_WindowFromHandle(OS_Handle handle)
{
    APP_Window *result = 0;
    for(APP_Window *w = app_state->first_window; w != 0; w = w->next)
    {
        if(OS_HandleMatch(w->handle, handle))
        {
            result = w;
            break;
        }
    }
    return result;
}
