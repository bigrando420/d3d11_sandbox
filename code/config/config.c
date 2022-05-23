////////////////////////////////
//~ rjf: Globals

global CFG_State *cfg_state = 0;

////////////////////////////////
//~ rjf: Functions

engine_function U64
CFG_HashFromString(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}

engine_function CFG_CachedFile *
CFG_CachedFileFromPath(String8 path)
{
    U64 hash = CFG_HashFromString(path);
    U64 slot = hash % cfg_state->table_size;
    CFG_CachedFile *cfg_node = 0;
    for(CFG_CachedFile *n = cfg_state->table[slot]; n != 0; n = n->next)
    {
        if(Str8Match(n->path, path, 0))
        {
            cfg_node = n;
            break;
        }
    }
    if(path.size != 0 && cfg_node == 0)
    {
        cfg_node = PushArrayZero(cfg_state->arena, CFG_CachedFile, 1);
        cfg_node->next = cfg_state->table[slot];
        cfg_state->table[slot] = cfg_node;
        cfg_node->arena = M_ArenaAlloc(Gigabytes(1));
        cfg_node->path = PushStr8Copy(cfg_state->arena, path);
        cfg_node->root = MD_NilNode();
    }
    if(cfg_node != 0 && cfg_node->last_checked_frame < cfg_state->frame_index)
    {
        OS_FileAttributes attribs = OS_FileAttributesFromPath(path);
        cfg_node->last_checked_frame = cfg_state->frame_index;
        if(attribs.last_modified != cfg_node->last_modified_time)
        {
            M_ArenaClear(cfg_node->arena);
            String8 file = OS_LoadEntireFile(cfg_node->arena, path).string;
            if(file.size != 0)
            {
                cfg_node->root = MD_ParseWholeString(cfg_node->arena, MDFromStr8(path), MDFromStr8(file)).node;
                cfg_node->last_modified_time = attribs.last_modified;
                cfg_node->generation += 1;
                cfg_node->user_data = 0;
            }
        }
    }
    return cfg_node;
}

engine_function CFG_NodeFilePair
CFG_NodeFilePairFromKey(String8 key, B32 alt_file_possible)
{
    CFG_NodeFilePair result = {0};
    result.node = MD_NilNode();
    
    // rjf: grab project file
    CFG_CachedFile *project_file = 0;
    if(cfg_state->project_path.size != 0)
    {
        project_file = CFG_CachedFileFromPath(cfg_state->project_path);
    }
    
    // rjf: grab config file
    CFG_CachedFile *config_file = 0;
    if(cfg_state->config_path.size != 0)
    {
        config_file = CFG_CachedFileFromPath(cfg_state->config_path);
    }
    
    // rjf: search files to get to root node
    CFG_CachedFile *files_to_check[] = {project_file, config_file};
    for(U64 idx = 0; idx < ArrayCount(files_to_check); idx += 1)
    {
        if(files_to_check[idx])
        {
            result.node = MD_ChildFromString(files_to_check[idx]->root, MDFromStr8(key), MD_StringMatchFlag_CaseInsensitive);
            if(!MD_NodeIsNil(result.node))
            {
                result.file = files_to_check[idx];
                break;
            }
        }
    }
    
    // rjf: with certain config options, the user can specify a path to another
    // file
    if(alt_file_possible && MD_ChildCountFromNode(result.node) == 1 &&
       result.node->first_child->string.size != 0 && MD_ChildCountFromNode(result.node->first_child) == 0)
    {
        String8 path = Str8FromMD(result.node->first_child->string);
        CFG_CachedFile *alt_file = CFG_CachedFileFromPath(path);
        MD_Node *alt_candidate = alt_file->root;
        if(!MD_NodeIsNil(alt_candidate))
        {
            result.node = alt_candidate;
            result.file = alt_file;
        }
    }
    
    return result;
}

engine_function MD_Node *
CFG_NodeFromKey(String8 key, B32 alt_file_possible)
{
    return CFG_NodeFilePairFromKey(key, alt_file_possible).node;
}

engine_function B32
CFG_B32FromKey(String8 key)
{
    MD_Node *node = CFG_NodeFromKey(key, 0);
    String8 value_string = Str8FromMD(node->first_child->string);
    B32 result = (Str8Match(value_string, Str8Lit("1"), 0) ||
                  Str8Match(value_string, Str8Lit("true"), MatchFlag_CaseInsensitive) ||
                  Str8Match(value_string, Str8Lit("yes"), MatchFlag_CaseInsensitive) ||
                  Str8Match(value_string, Str8Lit("y"), MatchFlag_CaseInsensitive));
    return result;
}

engine_function S64
CFG_S64FromKey(String8 key)
{
    MD_Node *node = CFG_NodeFromKey(key, 0);
    String8 value_string = Str8FromMD(node->first_child->string);
    S64 result = CStyleIntFromStr8(value_string);
    return result;
}

engine_function String8
CFG_ConfigPath(void)
{
    return cfg_state->config_path;
}

engine_function String8
CFG_ProjectPath(void)
{
    return cfg_state->project_path;
}

engine_function void
CFG_SetConfigPath(String8 path)
{
    M_ArenaClear(cfg_state->config_path_arena);
    cfg_state->config_path = PushStr8Copy(cfg_state->config_path_arena, path);
}

engine_function void
CFG_SetProjectPath(String8 path)
{
    M_ArenaClear(cfg_state->project_path_arena);
    cfg_state->project_path = PushStr8Copy(cfg_state->project_path_arena, path);
}

engine_function void
CFG_BeginFrame(void)
{
    cfg_state->frame_index += 1;
}

engine_function void
CFG_EndFrame(void)
{
}

engine_function void
CFG_Init(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(4));
    cfg_state = PushArrayZero(arena, CFG_State, 1);
    cfg_state->arena = arena;
    cfg_state->table_size = 4096;
    cfg_state->table = PushArrayZero(arena, CFG_CachedFile *, cfg_state->table_size);
    cfg_state->config_path_arena = M_ArenaAlloc(Kilobytes(1));
    cfg_state->project_path_arena = M_ArenaAlloc(Kilobytes(1));
    
    // rjf: set default config path
    M_Temp scratch = GetScratch(0, 0);
    String8 binary_path = OS_GetSystemPath(scratch.arena, OS_SystemPath_Binary);
    String8 config_path = PushStr8F(scratch.arena, "%S/config.tsc", binary_path);
    CFG_SetConfigPath(config_path);
    ReleaseScratch(scratch);
}
