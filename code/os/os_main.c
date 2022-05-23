////////////////////////////////
//~ rjf: Helpers

engine_function OS_Handle
OS_HandleZero(void)
{
    OS_Handle handle = {0};
    return handle;
}

engine_function B32
OS_HandleMatch(OS_Handle a, OS_Handle b)
{
    return a.u64[0] == b.u64[0];
}

engine_function String8
OS_NormalizedPathFromString(M_Arena *arena, String8 string)
{
    M_Temp scratch = GetScratch(&arena, 1);
    String8 slash_splits[] =
    {
        Str8LitComp("/"),
        Str8LitComp("\\"),
    };
    
    // rjf: sanitize whitespace
    string = Str8SkipChopWhitespace(string);
    
    // rjf: split by slashes
    String8List slash_separated_parts = StrSplit8(scratch.arena, string, ArrayCount(slash_splits), slash_splits);
    
    // rjf: form new list with only relevant parts
    String8List sanitized_parts = {0};
    for(String8Node *n = slash_separated_parts.first; n != 0; n = n->next)
    {
        if(n->string.size != 0 &&
           (n != slash_separated_parts.first || !Str8Match(n->string, Str8Lit("."), 0)))
        {
            Str8ListPush(scratch.arena, &sanitized_parts, n->string);
        }
    }
    
    // rjf: figure out if the passed string looks like a relative path
    B32 is_relative = 0;
    if(sanitized_parts.node_count != 0 &&
       (sanitized_parts.first->string.size > 0 && sanitized_parts.first->string.str[0] == '.') ||
       (sanitized_parts.first->string.size > 1 && sanitized_parts.first->string.str[1] != ':'))
    {
        is_relative = 1;
    }
    
    // rjf: build list of unsanitized absolute parts of path
    String8List absolute_parts = sanitized_parts;
    if(is_relative)
    {
        String8 process_path = OS_GetSystemPath(scratch.arena, OS_SystemPath_Current);
        String8List process_path_parts = StrSplit8(scratch.arena, process_path, ArrayCount(slash_splits), slash_splits);
        Str8ListConcat(&process_path_parts, &absolute_parts);
        absolute_parts = process_path_parts;
    }
    
    // rjf: collapse ..'s
    String8 *final_parts = PushArrayZero(scratch.arena, String8, absolute_parts.node_count);
    {
        // rjf: fill array
        {
            U64 idx = 0;
            for(String8Node *n = absolute_parts.first; n != 0; n = n->next, idx += 1)
            {
                final_parts[idx] = n->string;
            }
        }
        
        // rjf: use ..'s to eliminate folders
        for(U64 idx = 0; idx < absolute_parts.node_count; idx += 1)
        {
            if(Str8Match(final_parts[idx], Str8Lit(".."), 0))
            {
                for(U64 idx2 = idx-1; 0 <= idx2 && idx2 < absolute_parts.node_count; idx2 -= 1)
                {
                    if(final_parts[idx2].size != 0)
                    {
                        MemoryZeroStruct(&final_parts[idx]);
                        MemoryZeroStruct(&final_parts[idx2]);
                        break;
                    }
                }
            }
        }
    }
    
    // rjf: build list of final parts
    String8List final_parts_list = {0};
    for(U64 idx = 0; idx < absolute_parts.node_count; idx += 1)
    {
        if(final_parts[idx].size != 0)
        {
            Str8ListPush(scratch.arena, &final_parts_list, final_parts[idx]);
        }
    }
    
    // rjf: join
    StringJoin join = {0};
    join.sep = Str8Lit("/");
    String8 result = Str8ListJoin(arena, final_parts_list, &join);
    
    ReleaseScratch(scratch);
    return result;
}
