#include "engine_bundles/engine_full.h"

#include "os/os_entry_point.h"
#include "os/os_entry_point.c"

////////////////////////////////
//~ rjf: Main Calls

#define WINDOW_X 1280
#define WINDOW_Y 720
#define PIXEL_COUNT WINDOW_X * WINDOW_Y

typedef struct S_State S_State;
struct S_State
{
    M_Arena *permanent_arena;
    
    String8 cpu_data;
    
    U8 pixels[PIXEL_COUNT];
};

function S_State *
S_Open(APP_Window *window)
{
    M_Arena *arena = M_ArenaAllocDefault();
    S_State *state = PushStruct(arena, S_State);
    state->permanent_arena = arena;
    
    for (int i = 0; i < PIXEL_COUNT; i++)
    {
        U8 *pixel = &state->pixels[i];
        if (i % 2 == 0)
            *pixel = 1;
    }
    
    state->cpu_data = Str8(state->pixels, PIXEL_COUNT);
    
    return state;
}

function void
S_Close(APP_Window *window, S_State *state)
{
    M_ArenaRelease(state->permanent_arena);
}

function void
S_Update(APP_Window *window, OS_EventList *events, S_State *state)
{
    /* 
        local_persist Vec4F32 color = {1, 0, 0, 1};
        if(OS_KeyPress(events, window->handle, OS_Key_A, 0))
        {
            color = V4F32(0, 1, 0, 1);
        }
        
        DR_Bucket bucket = {0};
        {
            Vec2F32 mouse = OS_MouseFromWindow(window->handle);
            DR_Rect(&bucket, R2F32(mouse, Add2F32(mouse, V2F32(30, 30))), color);
        }
        DR_Submit(window->window_equip, bucket.cmds);
     */
    
    // NOTE(rjf): Example of explicitly using the low-level "R_" API for
    // just sending data to the rendering backend, instead of using the
    // higher level "DR_" (draw) API @api_notes
    M_Temp scratch = GetScratch(0, 0);
    R_CmdList my_low_level_commands = {0};
    R_Cmd cmd = {0};
    {
        cmd.kind = R_CmdKind_RayTracer;
        cmd.cpu_data = state->cpu_data;
    }
    R_CmdListPush(scratch.arena, &my_low_level_commands, &cmd);
    DR_Submit(window->window_equip, my_low_level_commands);
    ReleaseScratch(scratch);
    
    // TODO(randy): figure out how to render this data using d3d11
    // just take a look at minimal d3d11 and the docs to figure out how to pass this to the shader properly
}

////////////////////////////////
//~ rjf: Entry Points

function void
APP_Repaint(void)
{
    //- rjf: begin frame
    APP_BeginFrame();
    CFG_BeginFrame();
    OS_EventList events = OS_GetEvents(APP_FrameArena());
    
    //- rjf: update all windows
    for(APP_Window *window = app_state->first_window; window != 0; window = window->next)
    {
        if(window->initialized == 0)
        {
            window->initialized = 1;
            window->user.data = window->user.hooks.Open(window);
            OS_WindowFirstPaint(window->handle);
        }
        app_state->r_backend.Start(app_state->r_os_equip, window->window_equip, Vec2S64FromVec(Dim2F32(OS_ClientRectFromWindow(window->handle))));
        window->user.hooks.Update(window, &events, window->user.data);
        app_state->r_backend.Finish(app_state->r_os_equip, window->window_equip);
    }
    
    //- rjf: handle windows closing
    for(OS_Event *e = events.first; e != 0; e = e->next)
    {
        if(e->kind == OS_EventKind_WindowClose)
        {
            APP_Window *window = APP_WindowFromHandle(e->window);
            window->user.hooks.Close(window, window->user.data);
            APP_WindowClose(window);
        }
    }
    
    //- rjf: end frame
    CFG_EndFrame();
    APP_EndFrame();
}

function void
APP_EntryPoint(void)
{
    //- rjf: init
    TCTX tctx = MakeTCTX();
    SetTCTX(&tctx);
    OS_Init();
    OS_InitGfx(APP_Repaint);
    APP_Init();
    DR_Init(app_state->r_backend, app_state->r_os_equip);
    C_Init();
    VIN_Init();
    CFG_Init();
    
    //- rjf: open window
    APP_WindowUserHooks hooks =
    {
        (APP_WindowUserOpenFunction *)S_Open,
        (APP_WindowUserCloseFunction *)S_Close,
        (APP_WindowUserUpdateFunction *)S_Update,
    };
    APP_WindowOpen(Str8Lit("[Telescope] Randall's Scratch"), V2S64(1280, 720), hooks);
    
    //- rjf: loop
    for(;!APP_Quit();)
    {
        APP_Repaint();
    }
    
    //- rjf: quit
    C_Quit();
}
