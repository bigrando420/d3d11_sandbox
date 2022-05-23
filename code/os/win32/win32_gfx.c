#include <uxtheme.h>
#undef DeleteFile
#undef IsMaximized
#include <dwmapi.h>

#define W32_GraphicalWindowClassName L"ApplicationWindowClass"

////////////////////////////////
//~ rjf: Globals

#if ENGINE
W32_GetDpiForWindowType* w32_GetDpiForWindow = 0;
W32_Window *w32_first_window = 0;
W32_Window *w32_last_window = 0;
W32_Window *w32_free_window = 0;
OS_CursorKind w32_cursor_kind = OS_CursorKind_Null;
HWND w32_global_hwnd = 0;
HDC w32_global_hdc = 0;
M_Arena *w32_events_arena = 0;
OS_EventList *w32_events_list = 0;
WNDCLASSW w32_window_class = {0};
F32 w32_refresh_rate = 0;
OS_RepaintFunction *w32_repaint = 0;
#endif

////////////////////////////////
//~ rjf: Drag + Drop

#if LANG_CPP
function void
W32_DragDropInit(HWND window_handle)
{
    // TODO(rjf): no implementation for C++
}
#else

#define W32_IUNKNOWN_QUERYINTERFACE(name) HRESULT STDMETHODCALLTYPE name(void *this_, REFIID riid, void **ppv)
#define W32_IUNKNOWN_ADDREF(name) ULONG STDMETHODCALLTYPE name(void *this_)
#define W32_IUNKNOWN_RELEASE(name) ULONG STDMETHODCALLTYPE name(void *this_)
typedef W32_IUNKNOWN_QUERYINTERFACE(W32_QueryInterface);
typedef W32_IUNKNOWN_ADDREF(W32_AddRef);
typedef W32_IUNKNOWN_RELEASE(W32_Release);
#define W32_DRAG_ENTER(name) HRESULT STDMETHODCALLTYPE name(IDropTarget *this_, IDataObject *p_data_obj, DWORD key_state, POINTL pt, DWORD *pdw_effect)
#define W32_DRAG_OVER(name) HRESULT STDMETHODCALLTYPE name(IDropTarget *this_, DWORD key_state, POINTL pt, DWORD  *pdw_effect)
#define W32_DRAG_LEAVE(name) HRESULT STDMETHODCALLTYPE name(IDropTarget *this_)
#define W32_DRAG_DROP(name) HRESULT STDMETHODCALLTYPE name(IDropTarget *this_, IDataObject *p_data_obj, DWORD key_state, POINTL pt, DWORD *pdw_effect)
typedef W32_DRAG_ENTER(W32_DragEnterHook);
typedef W32_DRAG_OVER(W32_DragOverHook);
typedef W32_DRAG_LEAVE(W32_DragLeaveHook);
typedef W32_DRAG_DROP(W32_DragDropHook);

function W32_IUNKNOWN_QUERYINTERFACE(W32_DragDrop_QueryInterface)
{
    HRESULT result = NOERROR;
    if(!IsEqualIID(riid, &IID_IDropTarget) &&
       !IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = 0;
        result = E_NOINTERFACE;
    }
    else
    {
        *ppv = this_;
        ((IDropTarget *)this_)->lpVtbl->AddRef(this_);
    }
    return result;
}

function W32_IUNKNOWN_ADDREF(W32_DragDrop_AddRef)
{
    return 1;
}

function W32_IUNKNOWN_RELEASE(W32_DragDrop_Release)
{
    return 1;
}

function W32_DRAG_ENTER(W32_DragDrop_DragEnter)
{
    *pdw_effect = DROPEFFECT_COPY;
    return S_OK;
}

function W32_DRAG_OVER(W32_DragDrop_DragOver)
{
    *pdw_effect = DROPEFFECT_COPY;
    return S_OK;
}

function W32_DRAG_LEAVE(W32_DragDrop_DragLeave)
{
    return S_OK;
}

function W32_DRAG_DROP(W32_DragDrop_Drop)
{
    *pdw_effect = DROPEFFECT_COPY;
    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;
    if(SUCCEEDED(VTblCall(p_data_obj, GetData, &fmte, &stgm)))
    {
        HDROP hdrop = (HDROP)GlobalLock(stgm.hGlobal);
        UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
        char file_path[4096];
        for(UINT i = 0; i < file_count; i++)
        {
            DragQueryFile(hdrop, i, file_path, sizeof(file_path));
            //OS_PushEvent(OS_DropFileEventInitialize(cursor_pos, PushStringF(&global_frame_arena, "%s", file_path)));
            // TODO(rjf): What about pushing strings!?
            // TODO(rjf): hwnd?
            // W32_PushEvent(OS_EventKind_DropFile, hwnd);
        }
        GlobalUnlock(stgm.hGlobal);
        ReleaseStgMedium(&stgm);
    }
    return S_OK;
}

function void
W32_DragDropInit(HWND window_handle)
{
    local_persist IDropTargetVtbl drag_drop_vtable =
    {
        W32_DragDrop_QueryInterface,
        W32_DragDrop_AddRef,
        W32_DragDrop_Release,
        W32_DragDrop_DragEnter,
        W32_DragDrop_DragOver,
        W32_DragDrop_DragLeave,
        W32_DragDrop_Drop,
    };
    local_persist IDropTarget drag_drop = { &drag_drop_vtable };
    HRESULT result = RegisterDragDrop(window_handle, &drag_drop);
}

#endif

////////////////////////////////
//~ rjf: Helpers

engine_function OS_Handle
W32_HandleFromWindow(W32_Window *window)
{
    OS_Handle handle = {0};
    handle.u64[0] = (U64)window;
    return handle;
}

engine_function W32_Window *
W32_WindowFromHandle(OS_Handle handle)
{
    W32_Window *w = (W32_Window *)handle.u64[0];
    return w;
}

engine_function W32_Window *
W32_WindowOpen(String8 title, Vec2S64 size)
{
    //- rjf: allocate window
    W32_Window *window = w32_free_window;
    {
        if(window)
        {
            w32_free_window = w32_free_window->next;
            MemoryZeroStruct(w32_free_window);
        }
        else
        {
            window = PushArrayZero(w32_perm_arena, W32_Window, 1);
        }
        DLLPushBack(w32_first_window, w32_last_window, window);
    }
    
    //- rjf: open window
    HWND hwnd = 0;
    HDC hdc = 0;
    {
        M_Temp scratch = GetScratch(0, 0);
        String16 title16 = Str16From8(scratch.arena, title);
        hwnd = CreateWindowW(W32_GraphicalWindowClassName, (LPCWSTR)title16.str, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, size.x, size.y, 0, 0, w32_hinstance, 0);
        hdc = GetDC(hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        W32_DragDropInit(hwnd);
        ReleaseScratch(scratch);
    }
    
    //- rjf: fill window
    {
        window->hwnd = hwnd;
        window->hdc = hdc;
    }
    
    return window;
}

engine_function void
W32_WindowClose(W32_Window *window)
{
    DLLRemove(w32_first_window, w32_last_window, window);
    StackPush(w32_free_window, window);
    if(window->hdc)
    {
        ReleaseDC(window->hwnd, window->hdc);
    }
    if(window->hwnd)
    {
        DestroyWindow(window->hwnd);
    }
}

engine_function OS_Modifiers
W32_GetModifiers(void)
{
    OS_Modifiers modifiers = 0;
    if(GetKeyState(VK_CONTROL) & 0x8000)
    {
        modifiers |= OS_Modifier_Ctrl;
    }
    if(GetKeyState(VK_SHIFT) & 0x8000)
    {
        modifiers |= OS_Modifier_Shift;
    }
    if(GetKeyState(VK_MENU) & 0x8000)
    {
        modifiers |= OS_Modifier_Alt;
    }
    return modifiers;
}

////////////////////////////////
//~ rjf: Window Proc

function LRESULT
W32_WindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    OS_Event *event = 0;
    W32_Window *window = (W32_Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    OS_Handle window_handle = W32_HandleFromWindow(window);
    
    M_Temp scratch = GetScratch(&w32_events_arena, 1);
    OS_EventList fallback_event_list = {0};
    if(w32_events_arena == 0)
    {
        w32_events_arena = scratch.arena;
        w32_events_list = &fallback_event_list;
    }
    
    B32 is_release = 0;
    Axis2 scroll_axis = Axis2_Y;
    switch(message)
    {
        default:
        {
            result = DefWindowProcW(hwnd, message, w_param, l_param);
        }break;
        
        case WM_CLOSE:
        {
            event = PushArrayZero(w32_events_arena, OS_Event, 1);
            event->kind = OS_EventKind_WindowClose;
            event->window = window_handle;
        }break;
        
        case WM_KILLFOCUS:
        {
            event = PushArrayZero(w32_events_arena, OS_Event, 1);
            event->kind = OS_EventKind_WindowLoseFocus;
            event->window = window_handle;
            ReleaseCapture();
        }break;
        
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            ReleaseCapture();
            is_release = 1;
        }fallthrough;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if(is_release == 0)
            {
                SetCapture(hwnd);
            }
            
            OS_EventKind kind = is_release ? OS_EventKind_Release : OS_EventKind_Press;
            OS_Key key = OS_Key_MouseLeft;
            switch(message)
            {
                case WM_MBUTTONUP: case WM_MBUTTONDOWN: key = OS_Key_MouseMiddle; break;
                case WM_RBUTTONUP: case WM_RBUTTONDOWN: key = OS_Key_MouseRight; break;
            }
            
            event = PushArrayZero(w32_events_arena, OS_Event, 1);
            event->kind = kind;
            event->window = window_handle;
            event->key = key;
            event->position = OS_MouseFromWindow(window_handle);
        }break;
        
        case WM_MOUSEHWHEEL: scroll_axis = Axis2_X; goto scroll;
        case WM_MOUSEWHEEL:
        scroll:;
        {
            S16 wheel_delta = HIWORD(w_param);
            event = PushArrayZero(w32_events_arena, OS_Event, 1);
            event->kind = OS_EventKind_MouseScroll;
            event->window = window_handle;
            event->scroll.v[scroll_axis] = -(F32)wheel_delta;
        }break;
        
        case WM_SETCURSOR:
        {
            if(Contains2F32(OS_ClientRectFromWindow(window_handle), OS_MouseFromWindow(window_handle)) && w32_cursor_kind != OS_CursorKind_Null)
            {
                local_persist B32 table_initialized = 0;
                local_persist HCURSOR cursor_table[OS_CursorKind_COUNT];
                if(table_initialized == 0)
                {
                    table_initialized = 1;
                    cursor_table[OS_CursorKind_Pointer]                  = LoadCursorA(0, IDC_ARROW);
                    cursor_table[OS_CursorKind_Hand]                     = LoadCursorA(0, IDC_HAND);
                    cursor_table[OS_CursorKind_WestEast]                 = LoadCursorA(0, IDC_SIZEWE);
                    cursor_table[OS_CursorKind_NorthSouth]               = LoadCursorA(0, IDC_SIZENS);
                    cursor_table[OS_CursorKind_NorthEastSouthWest]       = LoadCursorA(0, IDC_SIZENESW);
                    cursor_table[OS_CursorKind_NorthWestSouthEast]       = LoadCursorA(0, IDC_SIZENWSE);
                    cursor_table[OS_CursorKind_AllCardinalDirections]    = LoadCursorA(0, IDC_SIZEALL);
                    cursor_table[OS_CursorKind_IBar]                     = LoadCursorA(0, IDC_IBEAM);
                    cursor_table[OS_CursorKind_Blocked]                  = LoadCursorA(0, IDC_NO);
                    cursor_table[OS_CursorKind_Loading]                  = LoadCursorA(0, IDC_WAIT);
                    cursor_table[OS_CursorKind_Pan]                      = LoadCursorA(0, IDC_SIZEALL);
                }
                if(w32_cursor_kind == OS_CursorKind_Hidden)
                {
                    ShowCursor(0);
                }
                else
                {
                    ShowCursor(1);
                    SetCursor(cursor_table[w32_cursor_kind]);
                }
            }
            else
            {
                result = DefWindowProcW(hwnd, message, w_param, l_param);
            }
        }break;
        
        case WM_SYSKEYDOWN: case WM_SYSKEYUP:
        {
            DefWindowProcW(hwnd, message, w_param, l_param);
        }fallthrough;
        case WM_KEYDOWN: case WM_KEYUP:
        {
            B32 was_down = !!(l_param & (1 << 30));
            B32 is_down =   !(l_param & (1 << 31));
            OS_EventKind kind = is_down ? OS_EventKind_Press : OS_EventKind_Release;
            
            local_persist OS_Key key_table[256] = {0};
            local_persist B32 key_table_initialized = 0;
            if(!key_table_initialized)
            {
                key_table_initialized = 1;
                
                for (U32 i = 'A', j = OS_Key_A; i <= 'Z'; i += 1, j += 1)
                {
                    key_table[i] = (OS_Key)j;
                }
                for (U32 i = '0', j = OS_Key_0; i <= '9'; i += 1, j += 1)
                {
                    key_table[i] = (OS_Key)j;
                }
                for (U32 i = VK_F1, j = OS_Key_F1; i <= VK_F24; i += 1, j += 1)
                {
                    key_table[i] = (OS_Key)j;
                }
                
                key_table[VK_ESCAPE]        = OS_Key_Esc;
                key_table[VK_OEM_3]         = OS_Key_GraveAccent;
                key_table[VK_OEM_MINUS]     = OS_Key_Minus;
                key_table[VK_OEM_PLUS]      = OS_Key_Equal;
                key_table[VK_BACK]          = OS_Key_Backspace;
                key_table[VK_TAB]           = OS_Key_Tab;
                key_table[VK_SPACE]         = OS_Key_Space;
                key_table[VK_RETURN]        = OS_Key_Enter;
                key_table[VK_CONTROL]       = OS_Key_Ctrl;
                key_table[VK_SHIFT]         = OS_Key_Shift;
                key_table[VK_MENU]          = OS_Key_Alt;
                key_table[VK_UP]            = OS_Key_Up;
                key_table[VK_LEFT]          = OS_Key_Left;
                key_table[VK_DOWN]          = OS_Key_Down;
                key_table[VK_RIGHT]         = OS_Key_Right;
                key_table[VK_DELETE]        = OS_Key_Delete;
                key_table[VK_PRIOR]         = OS_Key_PageUp;
                key_table[VK_NEXT]          = OS_Key_PageDown;
                key_table[VK_HOME]          = OS_Key_Home;
                key_table[VK_END]           = OS_Key_End;
                key_table[VK_OEM_2]         = OS_Key_ForwardSlash;
                key_table[VK_OEM_PERIOD]    = OS_Key_Period;
                key_table[VK_OEM_COMMA]     = OS_Key_Comma;
                key_table[VK_OEM_7]         = OS_Key_Quote;
                key_table[VK_OEM_4]         = OS_Key_LeftBracket;
                key_table[VK_OEM_6]         = OS_Key_RightBracket;
                key_table[VK_INSERT]        = OS_Key_Insert;
                key_table[VK_OEM_1]         = OS_Key_Semicolon;
            }
            
            OS_Key key = OS_Key_Null;
            if(w_param < ArrayCount(key_table))
            {
                key = key_table[w_param];
            }
            
            event = PushArrayZero(w32_events_arena, OS_Event, 1);
            event->kind = kind;
            event->window = window_handle;
            event->key = key;
        }break;
        
        case WM_SYSCOMMAND:
        {
            switch (w_param)
            {
                case SC_CLOSE:
                {
                    event = PushArrayZero(w32_events_arena, OS_Event, 1);
                    event->kind = OS_EventKind_WindowClose;
                    event->window = window_handle;
                } break;
                case SC_KEYMENU:
                {}break;
                
                default:
                {
                    result = DefWindowProcW(hwnd, message, w_param, l_param);
                }break;
            }
        }break;
        
        case WM_CHAR: case WM_SYSCHAR:
        {
            U32 char_input = w_param;
            if (char_input == '\r')
            {
                char_input = '\n';
            }
            if((char_input >= 32 && char_input != 127) || char_input == '\t' || char_input == '\n')
            {
                event = PushArrayZero(w32_events_arena, OS_Event, 1);
                event->kind = OS_EventKind_Text;
                event->window = window_handle;
                event->character = char_input;
            }
        }break;
        
        case WM_DPICHANGED:
        {
            F32 new_dpi = (F32)w_param;
            result = DefWindowProcW(hwnd, message, w_param, l_param);
        }break;
        
        case WM_PAINT:
        {
            if(w32_repaint)
            {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                w32_repaint();
                EndPaint(hwnd, &ps);
            }
            result = DefWindowProcW(hwnd, message, w_param, l_param);
        }break;
        
    }
    
    if(event)
    {
        event->modifiers = W32_GetModifiers();
        DLLPushBack(w32_events_list->first, w32_events_list->last, event);
        w32_events_list->count += 1;
    }
    
    ReleaseScratch(scratch);
    return result;
}

////////////////////////////////
//~ rjf: @os_per_backend Main API

engine_function void
OS_InitGfx(OS_RepaintFunction *repaint)
{
    SetProcessDPIAware();
    
    //- rjf: load DPI fallback
    {
        HMODULE user32 = LoadLibraryA("user32.dll");
        w32_GetDpiForWindow = (W32_GetDpiForWindowType*)GetProcAddress(user32, "GetDpiForWindow");
        FreeLibrary(user32);
    }
    
    //- rjf: initialize window class
    {
        WNDCLASSW window_class = {0};
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = W32_WindowProc;
        window_class.hInstance = w32_hinstance;
        window_class.lpszClassName = W32_GraphicalWindowClassName;
        window_class.hCursor = LoadCursor(0, IDC_ARROW);
        RegisterClassW(&window_class);
        w32_window_class = window_class;
    }
    
    //- rjf: make global invisible window
    {
        w32_global_hwnd = CreateWindowW(W32_GraphicalWindowClassName,
                                        L"",
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        100, 100,
                                        0, 0,
                                        w32_hinstance, 0);
        w32_global_hdc = GetDC(w32_global_hwnd);
    }
    
    
    //- rjf: find refresh rate
    {
        w32_refresh_rate = 60.f;
        DEVMODEA device_mode = {0};
        if(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &device_mode))
        {
            w32_refresh_rate = (float)device_mode.dmDisplayFrequency;
        }
    }
    
    //- rjf: initialize hooks
    w32_repaint = repaint;
    
}

engine_function F32
OS_DefaultRefreshRate(void)
{
    return w32_refresh_rate;
}

////////////////////////////////
//~ rjf: @os_per_backend Windows

engine_function OS_Handle
OS_WindowOpen(String8 title, Vec2S64 size)
{
    W32_Window *w = W32_WindowOpen(title, size);
    return W32_HandleFromWindow(w);
}

engine_function void
OS_WindowClose(OS_Handle handle)
{
    W32_Window *w = W32_WindowFromHandle(handle);
    W32_WindowClose(w);
}

engine_function void
OS_WindowSetTitle(OS_Handle handle, String8 title)
{
    M_Temp scratch = GetScratch(0, 0);
    W32_Window *w = W32_WindowFromHandle(handle);
    String16 title16 = Str16From8(scratch.arena, title);
    SetWindowTextW(w->hwnd, (WCHAR *)title16.str);
    ReleaseScratch(scratch);
}

engine_function B32
OS_WindowIsMaximized(OS_Handle handle)
{
    B32 result = 0;
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window)
    {
        result = !!(IsZoomed(window->hwnd));
    }
    return result;
}

engine_function void
OS_WindowMinimize(OS_Handle handle)
{
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window)
    {
        ShowWindow(window->hwnd, SW_MINIMIZE);
    }
}

engine_function void
OS_WindowMaximize(OS_Handle handle)
{
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window)
    {
        ShowWindow(window->hwnd, SW_MAXIMIZE);
    }
}

engine_function B32
OS_WindowIsFocused(OS_Handle handle)
{
    B32 result = 0;
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        result = GetForegroundWindow() == window->hwnd;
    }
    return result;
}

engine_function void
OS_WindowFullscreen(OS_Handle handle)
{
    W32_Window *window = W32_WindowFromHandle(handle);
    HWND hwnd = window->hwnd;
    if(window->last_window_placement_initialized == 0)
    {
        window->last_window_placement_initialized = 1;
        window->last_window_placement.length = sizeof(WINDOWPLACEMENT);
    }
    
    DWORD window_style = GetWindowLong(hwnd, GWL_STYLE);
    if(window_style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        if(GetWindowPlacement(hwnd, &window->last_window_placement) &&
           GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY),
                          &monitor_info))
        {
            SetWindowLong(hwnd, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right -
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom -
                         monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(hwnd, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &window->last_window_placement);
        SetWindowPos(hwnd, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

engine_function void
OS_WindowFirstPaint(OS_Handle handle)
{
    M_Temp scratch = GetScratch(0, 0);
    OS_EventList evts = {0};
    w32_events_list = &evts;
    w32_events_arena = scratch.arena;
    W32_Window *window = W32_WindowFromHandle(handle);
    ShowWindow(window->hwnd, SW_SHOW);
    UpdateWindow(window->hwnd);
    ReleaseScratch(scratch);
}

engine_function void
OS_WindowSetIcon(OS_Handle handle, Vec2S32 size, String8 rgba_data)
{
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        S32 size_area = size.x*size.y;
        HICON icon = NULL;
        
        ICONINFO icon_info = {0};
        icon_info.fIcon = TRUE;
        
        M_Temp scratch = GetScratch(0, 0);
        U8 *bgra_data = PushArray(scratch.arena, U8, 4 * size_area);
        
        U8 *rgba = rgba_data.str;
        U8 *bgra = bgra_data;
        for (int i_pixel = 0; i_pixel < size_area; ++i_pixel, rgba += 4, bgra += 4)
        {
            bgra[0] = rgba[2];
            bgra[1] = rgba[1];
            bgra[2] = rgba[0];
            bgra[3] = rgba[3];
        }
        
        icon_info.hbmColor = CreateBitmap(size.x, size.y, 1, 32, bgra_data);
        if(icon_info.hbmColor)
        {
            icon_info.hbmMask = CreateCompatibleBitmap(window->hdc, size.x, size.y);
            if(icon_info.hbmMask)
            {
                icon = CreateIconIndirect(&icon_info);
                DeleteObject(icon_info.hbmMask);
            }
            DeleteObject(icon_info.hbmColor);
        }
        
        if(icon)
        {
            SendMessage(window->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
            SendMessage(window->hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
        }
        
        ReleaseScratch(scratch);
    }
}

engine_function Rng2F32
OS_RectFromWindow(OS_Handle handle)
{
    Rng2F32 rect = {0};
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        RECT w32_rect = {0};
        if(GetWindowRect(window->hwnd, &w32_rect))
        {
            rect.x0 = (F32)w32_rect.left;
            rect.y0 = (F32)w32_rect.top;
            rect.x1 = (F32)w32_rect.right;
            rect.y1 = (F32)w32_rect.bottom;
        }
    }
    return rect;
}

engine_function Rng2F32
OS_ClientRectFromWindow(OS_Handle handle)
{
    Rng2F32 rect = {0};
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        RECT w32_rect = {0};
        if(GetClientRect(window->hwnd, &w32_rect))
        {
            rect.x0 = (F32)w32_rect.left;
            rect.y0 = (F32)w32_rect.top;
            rect.x1 = (F32)w32_rect.right;
            rect.y1 = (F32)w32_rect.bottom;
        }
    }
    return rect;
}

engine_function F32
OS_DPIFromWindow(OS_Handle handle)
{
    F32 result = 96.f;
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        HWND wnd = window->hwnd;
        HDC dc = window->hdc;
        F32 dpi;
        if(w32_GetDpiForWindow == 0)
        {
            F32 x = (F32)GetDeviceCaps(dc, LOGPIXELSX);
            F32 y = (F32)GetDeviceCaps(dc, LOGPIXELSY);
            dpi = x;
        }
        else
        {
            dpi = w32_GetDpiForWindow(wnd);
        }
        result = dpi;
    }
    return result;
}

engine_function Vec2F32
OS_MouseFromWindow(OS_Handle handle)
{
    Vec2F32 result = V2F32(-100, -100);
    W32_Window *window = W32_WindowFromHandle(handle);
    if(window != 0)
    {
        POINT point;
        if (GetCursorPos(&point))
        {
            if (ScreenToClient(window->hwnd, &point))
            {
                result = V2F32(point.x, point.y);
            }
        }
    }
    return result;
}

////////////////////////////////
//~ rjf: @os_per_backend Events

engine_function OS_EventList
OS_GetEvents(M_Arena *arena)
{
    OS_EventList list = {0};
    w32_events_arena = arena;
    w32_events_list = &list;
    for(MSG message; PeekMessage(&message, 0, 0, 0, PM_REMOVE);)
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    w32_events_arena = 0;
    w32_events_list = 0;
    
    // TODO(rjf): debugging
    // {
    for(OS_Event *e = list.first; e != 0; e = e->next)
    {
        if(e->next == (void *)0x3030303030232323)
        {
            int x = 0;
        }
    }
    // }
    
    return list;
}

engine_function void
OS_EatEvent(OS_EventList *events, OS_Event *event)
{
    DLLRemove(events->first, events->last, event);
    events->count -= 1;
    event->kind = OS_EventKind_Null;
    
    // TODO(rjf): debugging
    // {
    if(event->next == (void *)0x3030303030232323)
    {
        int x = 0;
    }
    // }
}

////////////////////////////////
//~ rjf: @os_per_backend Cursors

engine_function void
OS_SetCursor(OS_CursorKind kind)
{
    w32_cursor_kind = kind;
}

////////////////////////////////
//~ rjf: @os_per_backend Clipboard

engine_function void
OS_SetClipboardText(String8 string)
{
    if(OpenClipboard(0))
    {
        EmptyClipboard();
        HANDLE string_copy_handle = GlobalAlloc(GMEM_MOVEABLE, string.size+1);
        if(string_copy_handle)
        {
            U8 *copy_buffer = (U8 *)GlobalLock(string_copy_handle);
            MemoryCopy(copy_buffer, string.str, string.size);
            copy_buffer[string.size] = 0;
            GlobalUnlock(string_copy_handle);
            SetClipboardData(CF_TEXT, string_copy_handle);
        }
        CloseClipboard();
    }
}

engine_function String8
OS_GetClipboardText(M_Arena *arena)
{
    String8 result = {0};
    if(IsClipboardFormatAvailable(CF_TEXT) &&
       OpenClipboard(0))
    {
        HANDLE data_handle = GetClipboardData(CF_TEXT);
        if(data_handle)
        {
            U8 *buffer = (U8 *)GlobalLock(data_handle);
            if(buffer)
            {
                U64 size = CalculateCStringLength(buffer);
                result = PushStr8Copy(arena, Str8(buffer, size));
                GlobalUnlock(data_handle);
            }
        }
        CloseClipboard();
    }
    return result;
}
