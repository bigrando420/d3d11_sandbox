////////////////////////////////
//~ rjf: Globals

#if ENGINE
HINSTANCE w32_hinstance = 0;
M_Arena *w32_perm_arena = 0;
B32 w32_got_system_info = 0;
SYSTEM_INFO w32_system_info = {0};
W32_Process *w32_free_process = 0;
W32_Thread *w32_free_thread = 0;
SRWLOCK w32_mutex = SRWLOCK_INIT;
HMODULE w32_advapi_dll = 0;
BOOL (*RtlGenRandom)(VOID *RandomBuffer, ULONG RandomBufferLength);
String8 w32_initial_path = {0};
LARGE_INTEGER w32_counts_per_second = {0};
#endif

////////////////////////////////
//~ rjf: Helpers

function void
W32_ReadWholeBlock(HANDLE file, void *data, U64 data_len)
{
    U8 *ptr = (U8*)data;
    U8 *opl = ptr + data_len;
    for(;;)
    {
        U64 unread = (U64)(opl - ptr);
        DWORD to_read = (DWORD)(ClampTop(unread, U32Max));
        DWORD did_read = 0;
        if (!ReadFile(file, ptr, to_read, &did_read, 0))
        {
            break;
        }
        ptr += did_read;
        if (ptr >= opl)
        {
            break;
        }
    }
}

function void
W32_WriteWholeBlock(HANDLE file, String8List data)
{
    for(String8Node *node = data.first; node != 0; node = node->next)
    {
        U8 *ptr = node->string.str;
        U8 *opl = ptr + node->string.size;
        for(;;)
        {
            U64 unwritten = (U64)(opl - ptr);
            DWORD to_write = (DWORD)(ClampTop(unwritten, U32Max));
            DWORD did_write = 0;
            if(!WriteFile(file, ptr, to_write, &did_write, 0))
            {
                goto fail_out;
            }
            ptr += did_write;
            if(ptr >= opl)
            {
                break;
            }
        }
    }
    fail_out:;
}

function OS_FileAttributes
W32_FileAttributesFromFindData(WIN32_FIND_DATAW find_data)
{
    OS_FileAttributes attributes = {0};
    if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        attributes.flags |= OS_FileFlag_Directory;
    }
    attributes.size = (((U64)find_data.nFileSizeHigh) << 32) | (find_data.nFileSizeLow);
    attributes.last_modified = (((U64)find_data.ftLastWriteTime.dwHighDateTime) << 32) | (find_data.ftLastWriteTime.dwLowDateTime);
    return attributes;
}

function W32_Process*
W32_ProcessAlloc(void)
{
    AcquireSRWLockExclusive(&w32_mutex);
    W32_Process *result = w32_free_process;
    if (result != 0)
    {
        StackPop(w32_free_process);
    }
    else
    {
        result = PushArray(w32_perm_arena, W32_Process, 1);
    }
    MemoryZeroStruct(result);
    ReleaseSRWLockExclusive(&w32_mutex);
    return result;
}

function void
W32_ProcessFree(W32_Process *process)
{
    AcquireSRWLockExclusive(&w32_mutex);
    StackPush(w32_free_process, process);
    ReleaseSRWLockExclusive(&w32_mutex);
}

function B32
W32_ProcessIsRunning(String8 process_name)
{
    B32 result = 0;
    
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    {
        if(Process32First(snapshot, &entry))
        {
            for(;Process32Next(snapshot, &entry);)
            {
                if(Str8Match(Str8C(entry.szExeFile), process_name, MatchFlag_CaseInsensitive))
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    CloseHandle(snapshot);
    
    return result;
}

////////////////////////////////
//~ rjf: Threads

function OS_Handle
W32_HandleFromThread(W32_Thread *thread)
{
    OS_Handle handle = {0};
    handle.u64[0] = (U64)thread;
    return handle;
}

function W32_Thread *
W32_ThreadFromHandle(OS_Handle handle)
{
    W32_Thread *thread = (W32_Thread *)handle.u64[0];
    return thread;
}

////////////////////////////////
//~ rjf: General Program API

engine_function void
OS_Init(void)
{
    w32_perm_arena = M_ArenaAlloc(Gigabytes(1));
    w32_advapi_dll = LoadLibraryA("advapi32.dll");
    if(w32_advapi_dll)
    {
        *(FARPROC *)&RtlGenRandom = GetProcAddress(w32_advapi_dll, "SystemFunction036");
    }
    QueryPerformanceFrequency(&w32_counts_per_second);
    w32_initial_path = OS_GetSystemPath(w32_perm_arena, OS_SystemPath_Current);
}

engine_function void
OS_Abort(void)
{
    ExitProcess(1);
}

engine_function String8
OS_GetSystemPath(M_Arena *arena, OS_SystemPath path)
{
    M_Temp scratch = GetScratch(&arena, 1);
    String8 result = {0};
    
    switch (path)
    {
        case OS_SystemPath_Initial:
        {
            Assert(w32_initial_path.str != 0);
            result = w32_initial_path;
        }break;
        
        case OS_SystemPath_Current:
        {
            DWORD length = GetCurrentDirectoryW(0, 0);
            U16 *memory = PushArrayZero(scratch.arena, U16, length + 1);
            length = GetCurrentDirectoryW(length + 1, (WCHAR*)memory);
            result = Str8From16(arena, Str16(memory, length));
        }break;
        
        case OS_SystemPath_Binary:
        {
            local_persist B32 first = 1;
            local_persist String8 name = {0};
            if (first)
            {
                first = 0;
                U64 size = Kilobytes(32);
                U16 *buffer = PushArrayZero(scratch.arena, U16, size);
                DWORD length = GetModuleFileNameW(0, (WCHAR*)buffer, size);
                name = Str8From16(scratch.arena, Str16(buffer, length));
                name = Str8ChopLastSlash(name);
                AcquireSRWLockExclusive(&w32_mutex);
                U8 *buffer8 = PushArrayZero(w32_perm_arena, U8, name.size);
                ReleaseSRWLockExclusive(&w32_mutex);
                MemoryCopy(buffer8, name.str, name.size);
                name.str = buffer8;
            }
            result = name;
        }break;
        
        case OS_SystemPath_AppData:
        {
            local_persist B32 first = 1;
            local_persist String8 name = {0};
            if(first)
            {
                first = 0;
                U64 size = Kilobytes(32);
                U16 *buffer = PushArrayZero(scratch.arena, U16, size);
                if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR*)buffer)))
                {
                    name = Str8From16(scratch.arena, Str16C(buffer));
                    AcquireSRWLockExclusive(&w32_mutex);
                    U8 *buffer8 = PushArrayZero(w32_perm_arena, U8, name.size);
                    ReleaseSRWLockExclusive(&w32_mutex);
                    MemoryCopy(buffer8, name.str, name.size);
                    name.str = buffer8;
                }
            }
            result = name;
        }break;
    }
    
    ReleaseScratch(scratch);
    
    return result;
}

////////////////////////////////
//~ rjf: Memory

engine_function U64
OS_PageSize(void)
{
    // TODO(rjf): @cleanup dedup with OS_LogicalProcessorCount
    if(w32_got_system_info == 0)
    {
        w32_got_system_info = 1;
        GetSystemInfo(&w32_system_info);
    }
    return w32_system_info.dwPageSize;
}

engine_function void *
OS_Reserve(U64 size)
{
    U64 gb_snapped_size = size;
    gb_snapped_size += Gigabytes(1) - 1;
    gb_snapped_size -= gb_snapped_size%Gigabytes(1);
    void *ptr = VirtualAlloc(0, gb_snapped_size, MEM_RESERVE, PAGE_NOACCESS);
    return ptr;
}

engine_function void
OS_Release(void *ptr)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

engine_function void
OS_Commit(void *ptr, U64 size)
{
    U64 page_snapped_size = size;
    page_snapped_size += OS_PageSize() - 1;
    page_snapped_size -= page_snapped_size%OS_PageSize();
    VirtualAlloc(ptr, page_snapped_size, MEM_COMMIT, PAGE_READWRITE);
}

engine_function void
OS_Decommit(void *ptr, U64 size)
{
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

////////////////////////////////
//~ rjf: Libraries

engine_function OS_Handle
OS_LibraryOpen(String8 path)
{
    M_Temp scratch = GetScratch(0, 0);
    String16 path16 = Str16From8(scratch.arena, path);
    HMODULE hmodule = LoadLibraryW((WCHAR*)path16.str);
    ReleaseScratch(scratch);
    OS_Handle handle = {0};
    handle.u64[0] = (U64)hmodule;
    return handle;
}

engine_function void
OS_LibraryClose(OS_Handle handle)
{
    HMODULE hmodule = (HMODULE)handle.u64[0];
    FreeLibrary(hmodule);
}

engine_function VoidFunction *
OS_LibraryLoadFunction(OS_Handle handle, String8 name)
{
    M_Temp scratch = GetScratch(0, 0);
    HMODULE hmodule = (HMODULE)handle.u64[0];
    String8 name_copy = PushStr8Copy(scratch.arena, name);
    VoidFunction *result = (VoidFunction *)GetProcAddress(hmodule, (char *)name_copy.str);
    ReleaseScratch(scratch);
    return result;
}

////////////////////////////////
//~ rjf: File System

engine_function OS_FileLoadResult
OS_LoadEntireFile(M_Arena *arena, String8 path)
{
    OS_FileLoadResult result = {0};
    
    DWORD desired_access = GENERIC_READ | GENERIC_WRITE;
    DWORD share_mode = 0;
    SECURITY_ATTRIBUTES security_attributes =
    {
        (DWORD)sizeof(SECURITY_ATTRIBUTES),
        0,
        0,
    };
    DWORD creation_disposition = OPEN_EXISTING;
    DWORD flags_and_attributes = 0;
    HANDLE template_file = 0;
    
    M_Temp scratch = GetScratch(&arena, 1);
    
    String16 path16 = Str16From8(scratch.arena, path);
    HANDLE file = CreateFileW((WCHAR*)path16.str,
                              desired_access,
                              share_mode,
                              &security_attributes,
                              creation_disposition,
                              flags_and_attributes,
                              template_file);
    
    if(file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size_int;
        if(GetFileSizeEx(file, &size_int) && size_int.QuadPart > 0)
        {
            U64 size = size_int.QuadPart;
            void *data = PushArray(arena, U8, size + 1);
            W32_ReadWholeBlock(file, data, size);
            ((U8*)data)[size] = 0;
            result.string = Str8((U8 *)data, size);
            result.success = 1;
        }
        else
        {
            result.success = 1;
            result.string.str = PushArrayZero(arena, U8, 1);
            result.string.size = 0;
        }
        CloseHandle(file);
    }
    
    ReleaseScratch(scratch);
    return result;
}

engine_function B32
OS_SaveToFile(String8 path, String8List data)
{
    B32 result = 0;
    HANDLE file = {0};
    {
        DWORD desired_access = GENERIC_READ | GENERIC_WRITE;
        DWORD share_mode = 0;
        SECURITY_ATTRIBUTES security_attributes = { (DWORD)sizeof(SECURITY_ATTRIBUTES) };
        DWORD creation_disposition = CREATE_ALWAYS;
        DWORD flags_and_attributes = 0;
        HANDLE template_file = 0;
        M_Temp scratch = GetScratch(0, 0);
        String16 path16 = Str16From8(scratch.arena, path);
        if((file = CreateFileW((WCHAR*)path16.str,
                               desired_access,
                               share_mode,
                               &security_attributes,
                               creation_disposition,
                               flags_and_attributes,
                               template_file)) != INVALID_HANDLE_VALUE)
        {
            result = 1;
            W32_WriteWholeBlock(file, data);
            CloseHandle(file);
        }
        ReleaseScratch(scratch);
    }
    return result;
}

engine_function void
OS_DeleteFile(String8 path)
{
    M_Temp scratch = GetScratch(0, 0);
    String16 path16 = Str16From8(scratch.arena, path);
    DeleteFileW((WCHAR *)path16.str);
    ReleaseScratch(scratch);
}

engine_function void
OS_MoveFile(String8 path, String8 new_path)
{
    M_Temp scratch = GetScratch(0, 0);
    String16 src_path16 = Str16From8(scratch.arena, path);
    String16 dst_path16 = Str16From8(scratch.arena, new_path);
    MoveFileW((WCHAR *)src_path16.str, (WCHAR *)dst_path16.str);
    ReleaseScratch(scratch);
}

engine_function B32
OS_CopyFile(String8 path, String8 new_path)
{
    M_Temp scratch = GetScratch(0, 0);
    String16 src_path16 = Str16From8(scratch.arena, path);
    String16 dst_path16 = Str16From8(scratch.arena, new_path);
    B32 result = CopyFileW((WCHAR *)src_path16.str, (WCHAR *)dst_path16.str, 0);
    ReleaseScratch(scratch);
    return result;
}

engine_function B32
OS_MakeDirectory(String8 path)
{
    M_Temp scratch = GetScratch(0, 0);
    String16 path16 = Str16From8(scratch.arena, path);
    B32 result = 1;
    if(!CreateDirectoryW((WCHAR *)path16.str, 0))
    {
        result = 0;
    }
    ReleaseScratch(scratch);
    return result;
}

engine_function OS_FileIter *
OS_FileIterBegin(M_Arena *arena, String8 path)
{
    M_Temp scratch = GetScratch(&arena, 1);
    path = Str8SkipChopWhitespace(path);
    U64 last_slash_pos = FindSubstr8(path, Str8Lit("/"), 0, MatchFlag_SlashInsensitive|MatchFlag_FindLast);
    if(last_slash_pos == path.size-1)
    {
        path = PushStr8F(scratch.arena, "%S/*", path);
    }
    
    String16 path16 = Str16From8(scratch.arena, path);
    W32_FileFindData *file_find_data = PushArrayZero(arena, W32_FileFindData, 1);
    file_find_data->handle = FindFirstFileW((WCHAR *)path16.str, &file_find_data->find_data);
    OS_FileIter *it = (OS_FileIter *)file_find_data;
    ReleaseScratch(scratch);
    return it;
}

engine_function B32
OS_FileIterNext(M_Arena *arena, OS_FileIter *it, OS_FileInfo *out_info)
{
    B32 result = 0;
    W32_FileFindData *file_find_data = (W32_FileFindData *)it;
    WIN32_FIND_DATAW find_data = {0};
    if(file_find_data->returned_first)
    {
        result = FindNextFileW(file_find_data->handle, &find_data);
    }
    else
    {
        result = file_find_data->handle != 0 && file_find_data->handle != INVALID_HANDLE_VALUE;
        find_data = file_find_data->find_data;
        file_find_data->returned_first = 1;
    }
    if(result)
    {
        String16 name16 = {0};
        name16.str = find_data.cFileName;
        name16.size = 0;
        for(U64 idx = 0; idx < MAX_PATH; idx += 1)
        {
            if(find_data.cFileName[idx] == 0)
            {
                break;
            }
            name16.size += 1;
        }
        MemoryZeroStruct(out_info);
        out_info->name = Str8From16(arena, name16);
        out_info->attributes = W32_FileAttributesFromFindData(find_data);
    }
    return result;
}

engine_function void
OS_FileIterEnd(OS_FileIter *it)
{
    W32_FileFindData *file_find_data = (W32_FileFindData *)it;
    FindClose(file_find_data->handle);
}

engine_function OS_FileAttributes
OS_FileAttributesFromPath(String8 path)
{
    WIN32_FIND_DATAW find_data = {0};
    M_Temp scratch = GetScratch(0, 0);
    String16 path16 = Str16From8(scratch.arena, path);
    HANDLE handle = FindFirstFileW((WCHAR *)path16.str, &find_data);
    FindClose(handle);
    OS_FileAttributes attributes = W32_FileAttributesFromFindData(find_data);
    ReleaseScratch(scratch);
    return attributes;
}

////////////////////////////////
//~ rjf: Time

engine_function U64
OS_TimeMicroseconds(void)
{
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    F64 time_in_seconds = ((F64)current_time.QuadPart)/((F64)w32_counts_per_second.QuadPart);
    U64 time_in_microseconds = (U64)(time_in_seconds * Million(1));
    return time_in_microseconds;
}

engine_function U64
OS_TimeCycles(void)
{
    U64 result = __rdtsc();
    return result;
}

engine_function void
OS_Sleep(U64 milliseconds)
{
    Sleep(milliseconds);
}

////////////////////////////////
//~ rjf: Threads & Synchronization Primitives

function DWORD
W32_ThreadFunctionBase(void *params)
{
    W32_Thread *thread = (W32_Thread *)params;
    TCTX tctx = MakeTCTX();
    SetTCTX(&tctx);
    thread->func(thread->params);
    return 0;
}

engine_function U64
OS_GetTID(void)
{
    return GetThreadId(0);
}

engine_function OS_Handle
OS_ThreadStart(void *params, OS_ThreadFunction *func)
{
    W32_Thread *thread = w32_free_thread;
    if(thread != 0)
    {
        w32_free_thread = w32_free_thread->next;
        MemoryZeroStruct(thread);
    }
    else
    {
        thread = PushArrayZero(w32_perm_arena, W32_Thread, 1);
    }
    thread->params = params;
    thread->func = func;
    thread->handle = CreateThread(0, 0, W32_ThreadFunctionBase, thread, 0, &thread->thread_id);
    OS_Handle result = W32_HandleFromThread(thread);
    return result;
}

engine_function void
OS_ThreadJoin(OS_Handle handle)
{
    W32_Thread *thread = W32_ThreadFromHandle(handle);
    if(thread->handle != 0)
    {
        WaitForSingleObject(thread->handle, INFINITE);
        CloseHandle(thread->handle);
    }
    StackPush(w32_free_thread, thread);
}

engine_function void
OS_ThreadDetach(OS_Handle handle)
{
    W32_Thread *thread = W32_ThreadFromHandle(handle);
    if(thread->handle != 0)
    {
        CloseHandle(thread->handle);
    }
    StackPush(w32_free_thread, thread);
}

engine_function OS_Handle
OS_SemaphoreAlloc(U32 max_count)
{
    max_count = ClampTop(max_count, U32Max/2);
    HANDLE handle = CreateSemaphoreA(0, 0, max_count, 0);
    OS_Handle result = {0};
    result.u64[0] = (U64)handle;
    return result;
}

engine_function void
OS_SemaphoreRelease(OS_Handle handle)
{
    HANDLE h = (HANDLE)handle.u64[0];
    CloseHandle(h);
}

engine_function void
OS_SemaphoreWait(OS_Handle handle, U32 max_milliseconds)
{
    HANDLE h = (HANDLE)handle.u64[0];
    WaitForSingleObject(h, (max_milliseconds == U32Max ? INFINITE : max_milliseconds));
}

engine_function void
OS_SemaphoreSignal(OS_Handle handle)
{
    HANDLE h = (HANDLE)handle.u64[0];
    ReleaseSemaphore(h, 1, 0);
}

////////////////////////////////
//~ rjf: Child Processes

engine_function OS_Handle
OS_ProcessLaunch(String8 command, String8 working_directory)
{
    W32_Process *w32_proc = W32_ProcessAlloc();
    
    M_Temp scratch = GetScratch(0, 0);
    
    //- rjf: convert inputs
    String8 command_prepped = PushStr8F(scratch.arena, "cmd.exe /C %.*s", Str8VArg(command));
    String16 command16 = Str16From8(scratch.arena, command_prepped);
    String16 dir16 = Str16From8(scratch.arena, working_directory);
    
    //- rjf: make security attributes
    SECURITY_ATTRIBUTES security = {sizeof(security)};
    security.bInheritHandle = TRUE;
    
    //- rjf: setup intermediates
    HANDLE child_write = INVALID_HANDLE_VALUE;
    HANDLE parent_read = INVALID_HANDLE_VALUE;
    HANDLE child_read = INVALID_HANDLE_VALUE;
    
    B32 success = 0;
    
    //- rjf: pipe
    if (CreatePipe(&parent_read, &child_write, &security, 0))
    {
        if (SetHandleInformation(parent_read, HANDLE_FLAG_INHERIT, 0))
        {
            
            //- rjf: nul file (child stdin)
            child_read = CreateFileA("nul",
                                     GENERIC_READ,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     &security,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
            
            //- rjf: process
            STARTUPINFOW startup = {sizeof(startup)};
            startup.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            startup.hStdInput = child_read;
            startup.hStdOutput = child_write;
            startup.hStdError = child_write;
            startup.wShowWindow = SW_HIDE;
            
            if(CreateProcessW(L"c:\\windows\\system32\\cmd.exe",
                              (WCHAR*)command16.str,
                              0, 0, TRUE, 0,
                              0,
                              (WCHAR*)dir16.str,
                              &startup,
                              &w32_proc->info))
            {
                success = 1;
                w32_proc->status.running = 1;
                w32_proc->parent_read = parent_read;
                
                CloseHandle(child_write);
                child_write = INVALID_HANDLE_VALUE;
                
                CloseHandle(child_read);
                child_read = INVALID_HANDLE_VALUE;
            }
        }
    }
    
    if (!success)
    {
        w32_proc->status.launch_failed = 1;
        
        if (child_write != INVALID_HANDLE_VALUE)
        {
            CloseHandle(child_write);
        }
        if (parent_read != INVALID_HANDLE_VALUE)
        {
            CloseHandle(parent_read);
        }
        if (child_read != INVALID_HANDLE_VALUE)
        {
            CloseHandle(child_read);
        }
    }
    
    ReleaseScratch(scratch);
    
    OS_Handle result = {0};
    result.u64[0] = IntFromPtr(w32_proc);
    return result;
}

engine_function void
OS_ProcessRelease(OS_Handle handle)
{
    W32_Process *w32_proc = (W32_Process *)PtrFromInt(handle.u64[0]);
    
    if(!w32_proc->status.launch_failed)
    {
        CloseHandle(w32_proc->info.hProcess);
        CloseHandle(w32_proc->info.hThread);
        CloseHandle(w32_proc->parent_read);
    }
    
    W32_ProcessFree(w32_proc);
}

engine_function String8
OS_ProcessReadOutput(M_Arena *arena, OS_Handle process)
{
    W32_Process *w32_proc = (W32_Process *)PtrFromInt(process.u64[0]);
    
    // NOTE(rjf): if the process has exited, save the exit code.
    B32 proc_exit = 0;
    if (w32_proc->status.running)
    {
        if (WaitForSingleObject(w32_proc->info.hProcess, 0) == WAIT_OBJECT_0)
        {
            proc_exit = 1;
            DWORD exit_code = 0;
            if (GetExitCodeProcess(w32_proc->info.hProcess, &exit_code))
            {
                w32_proc->status.exit_code = exit_code;
            }
        }
    }
    
    // NOTE(rjf): read anything that is ready on the pipe.
    String8 result = {0};
    B32 success = 0;
    
    if (w32_proc->status.running &&
        !w32_proc->status.launch_failed &&
        !w32_proc->status.read_failed &&
        !w32_proc->status.kill_failed)
    {
        DWORD read_size = 0;
        
        if (PeekNamedPipe(w32_proc->parent_read, 0, 0, 0, &read_size, 0))
        {
            if (read_size == 0)
            {
                success = 1;
            }
            else
            {
                result.str = PushArray(arena, U8, read_size);
                result.size = read_size;
                DWORD read_amount = 0;
                if (ReadFile(w32_proc->parent_read, result.str, read_size, &read_amount, 0))
                {
                    if (read_amount == read_size)
                    {
                        success = 1;
                    }
                }
            }
        }
    }
    
    if (!success)
    {
        w32_proc->status.read_failed = 1;
    }
    
    // NOTE(rjf): mark the process handle as not running after exit
    if (proc_exit)
    {
        w32_proc->status.running = 0;
    }
    
    return result;
}

engine_function void
OS_ProcessKill(OS_Handle process)
{
    W32_Process *w32_proc = (W32_Process *)PtrFromInt(process.u64[0]);
    B32 success = 0;
    if (w32_proc->status.running)
    {
        if (TerminateProcess(w32_proc->info.hProcess, 0))
        {
            success = 1;
        }
    }
    
    if (!success)
    {
        w32_proc->status.kill_failed = 1;
    }
    else
    {
        w32_proc->status.running = 0;
        w32_proc->status.was_killed = 1;
    }
}

engine_function U64
OS_PIDFromProcess(OS_Handle process)
{
    W32_Process *w32_proc = (W32_Process *)PtrFromInt(process.u64[0]);
    U64 result = w32_proc->info.dwProcessId;
    return result;
}

engine_function OS_ProcessStatus
OS_StatusFromProcess(OS_Handle process)
{
    W32_Process *w32_proc = (W32_Process *)PtrFromInt(process.u64[0]);
    OS_ProcessStatus result = w32_proc->status;
    return result;
}

////////////////////////////////
//~ rjf: Miscellaneous

engine_function void
OS_GetEntropy(void *data, U64 size)
{
#if 1
    // NOTE(rjf): Martins says that this method will be faster (as it does not require
    // opening a crypto provider)!
    RtlGenRandom(data, size);
#else
    HCRYPTPROV provider;
    if(CryptAcquireContextW(&provider,
                            NULL,
                            (LPCWSTR)L"Microsoft Base Cryptographic Provider v1.0",
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))
    {
        CryptGenRandom(provider, size, (BYTE*)data);
        CryptReleaseContext(provider, 0);
    }
#endif
}

engine_function void
OS_OpenCodeFileInDevTools(String8 path, int line)
{
    // NOTE(rjf): Open file in Remedy
    if(W32_ProcessIsRunning(Str8Lit("remedybg.exe")))
    {
        STARTUPINFO startup_info = {0};
        PROCESS_INFORMATION process_info = {0};
        startup_info.cb = sizeof(startup_info);
        char cmd_line[4096] = {0};
        snprintf(cmd_line, sizeof(cmd_line), "remedybg.exe open-file %.*s %i", Str8VArg(path), line);
        CreateProcessA(0, cmd_line, 0, 0, 0, DETACHED_PROCESS, 0, 0, &startup_info, &process_info);
    }
    
    // NOTE(rjf): Open file in VS
    if(W32_ProcessIsRunning(Str8Lit("devenv.exe")))
    {
        STARTUPINFO startup_info = {0};
        PROCESS_INFORMATION process_info = {0};
        startup_info.cb = sizeof(startup_info);
        char cmd_line[4096] = {0};
        snprintf(cmd_line, sizeof(cmd_line), "devenv /edit %.*s /command \"edit.goto %i\"", Str8VArg(path), line);
        CreateProcessA(0, cmd_line, 0, 0, 0, DETACHED_PROCESS, 0, 0, &startup_info, &process_info);
    }
}

////////////////////////////////
//~ rjf: @os_per_backend System Info

engine_function F32
OS_CaretBlinkTime(void)
{
    F32 seconds = GetCaretBlinkTime() / 1000.f;
    return seconds;
}

engine_function F32
OS_DoubleClickTime(void)
{
    F32 seconds = GetDoubleClickTime() / 1000.f;
    return seconds;
}

engine_function U64
OS_LogicalProcessorCount(void)
{
    if(w32_got_system_info == 0)
    {
        w32_got_system_info = 1;
        GetSystemInfo(&w32_system_info);
    }
    return w32_system_info.dwNumberOfProcessors;
}
