////////////////////////////////
//~ rjf: Generated Code

#include "generated/client.meta.c"

////////////////////////////////
//~ rjf: Globals

global C_State *c_state = 0;
global volatile B32 c_g_quit_signal = 0;

#if ENGINE
read_only C_Entity c_g_nil_entity =
{
    &c_g_nil_entity,
    &c_g_nil_entity,
    &c_g_nil_entity,
    &c_g_nil_entity,
    &c_g_nil_entity,
    0,
    0,
    C_EntityKind_Nil,
    0,
    {0},
    0,
};
read_only C_View c_g_nil_view =
{
    &c_g_nil_view,
    &c_g_nil_view,
    0,
};
read_only C_Panel c_g_nil_panel =
{
    &c_g_nil_panel,
    &c_g_nil_panel,
    &c_g_nil_panel,
    &c_g_nil_panel,
    &c_g_nil_panel,
    0,
};
#endif

////////////////////////////////
//~ rjf: Basic Helpers

engine_function U64
C_HashFromString(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}

engine_function U64
C_HashFromString_CaseInsensitive(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + CharToUpper(string.str[i]);
    }
    return result;
}

////////////////////////////////
//~ rjf: Handles

engine_function C_Handle
C_HandleZero(void)
{
    C_Handle handle = {0};
    return handle;
}

engine_function void
C_HandleListPush(M_Arena *arena, C_HandleList *list, C_Handle handle)
{
    C_HandleNode *node = PushArrayZero(arena, C_HandleNode, 1);
    node->handle = handle;
    QueuePush(list->first, list->last, node);
    list->count += 1;
}

////////////////////////////////
//~ rjf: Views

engine_function B32
C_ViewIsNil(C_View *view)
{
    return view == 0 || view == &c_g_nil_view;
}

engine_function C_Handle
C_HandleFromView(C_View *view)
{
    C_Handle handle = {0};
    handle.u64[0] = (U64)view;
    handle.u64[1] = view->generation;
    return handle;
}

engine_function C_View *
C_ViewFromHandle(C_Handle handle)
{
    C_View *view = (C_View *)handle.u64[0];
    if(!C_ViewIsNil(view) && view->generation != handle.u64[1])
    {
        view = &c_g_nil_view;
    }
    return view;
}

engine_function C_View *
C_ViewAlloc(void)
{
    C_View *view = c_state->free_view;
    if(C_ViewIsNil(view))
    {
        view = PushArrayZero(c_state->arena, C_View, 1);
        view->arena = M_ArenaAlloc(Megabytes(256));
    }
    else
    {
        StackPop(c_state->free_view);
        M_Arena *arena = view->arena;
        U64 generation = view->generation;
        MemoryZeroStruct(view);
        view->arena = arena;
        view->generation = generation + 1;
    }
    return view;
}

engine_function void
C_ViewRelease(C_View *view)
{
    if(!C_ViewIsNil(view))
    {
        StackPush(c_state->free_view, view);
        view->generation += 1;
    }
}

engine_function void
C_ViewSetCommandAndEntity(C_View *view, String8 string, C_Entity *entity)
{
    M_ArenaClear(view->arena);
    view->command = PushStr8Copy(view->arena, string);
    view->entity = entity;
}

////////////////////////////////
//~ rjf: Panel Tree

engine_function B32
C_PanelIsNil(C_Panel *panel)
{
    return panel == 0 || panel == &c_g_nil_panel;
}

engine_function C_Panel *
C_PanelAlloc(void)
{
    C_Panel *panel = c_state->free_panel;
    if(!C_PanelIsNil(panel))
    {
        StackPop(c_state->free_panel);
        U64 generation = panel->generation;
        MemoryZeroStruct(panel);
        panel->generation = generation+1;
    }
    else
    {
        panel = PushArrayZero(c_state->arena, C_Panel, 1);
    }
    panel->first = panel->last = panel->next = panel->prev = panel->parent = &c_g_nil_panel;
    panel->cursor = panel->mark = TE_PointMake(1, 1);
    panel->input.str = panel->input_buffer;
    panel->input.size = 0;
    panel->error.str = panel->error_buffer;
    panel->content_box = &ui_g_nil_box;
    panel->first_view = panel->last_view = panel->selected_view = &c_g_nil_view;
    panel->transient_view = C_ViewAlloc();
    return panel;
}

engine_function void
C_PanelRelease(C_Panel *panel)
{
    for(C_Panel *child = panel->first, *next = 0; !C_PanelIsNil(child); child = next)
    {
        next = child->next;
        C_PanelRelease(child);
    }
    for(C_View *view = panel->first_view, *next = 0; !C_ViewIsNil(view); view = next)
    {
        next = view->next;
        C_ViewRelease(view);
    }
    C_ViewRelease(panel->transient_view);
    StackPush(c_state->free_panel, panel);
    panel->generation += 1;
}

engine_function void
C_PanelInsert(C_Panel *parent, C_Panel *prev_child, C_Panel *new_child)
{
    DLLInsert_NPZ(parent->first, parent->last, prev_child, new_child, next, prev, C_PanelIsNil, C_PanelSetNil);
    parent->child_count += 1;
    new_child->parent = parent;
}

engine_function void
C_PanelRemove(C_Panel *parent, C_Panel *child)
{
    DLLRemove_NPZ(parent->first, parent->last, child, next, prev, C_PanelIsNil, C_PanelSetNil);
    child->next = child->prev = child->parent = &c_g_nil_panel;
    parent->child_count -= 1;
}

engine_function C_PanelRec
C_PanelRecDF(C_Panel *panel, U64 sib_off, U64 child_off)
{
    C_PanelRec rec = {0};
    if(!C_PanelIsNil(*MemberFromOffset(C_Panel **, panel, child_off)))
    {
        rec.next = *MemberFromOffset(C_Panel **, panel, child_off);
        rec.push_count = 1;
    }
    else if(!C_PanelIsNil(*MemberFromOffset(C_Panel **, panel, sib_off)))
    {
        rec.next = *MemberFromOffset(C_Panel **, panel, sib_off);
    }
    else
    {
        C_Panel *uncle = &c_g_nil_panel;
        for(C_Panel *p = panel->parent; !C_PanelIsNil(p); p = p->parent)
        {
            rec.pop_count += 1;
            if(!C_PanelIsNil(*MemberFromOffset(C_Panel **, p, sib_off)))
            {
                uncle = *MemberFromOffset(C_Panel **, p, sib_off);
                break;
            }
        }
        rec.next = uncle;
    }
    return rec;
}

engine_function Rng2F32
C_RectFromPanelChild(Rng2F32 parent_rect, C_Panel *parent, C_Panel *panel)
{
    Rng2F32 rect = parent_rect;
    if(!C_PanelIsNil(parent))
    {
        Vec2F32 parent_rect_size = Dim2F32(parent_rect);
        Axis2 axis = parent->split_axis;
        rect.p1.v[axis] = rect.p0.v[axis];
        for(C_Panel *child = parent->first; !C_PanelIsNil(child); child = child->next)
        {
            rect.p1.v[axis] += parent_rect_size.v[axis] * child->pct_of_parent;
            if(child == panel)
            {
                break;
            }
            rect.p0.v[axis] = rect.p1.v[axis];
        }
    }
    return rect;
}

engine_function Rng2F32
C_RectFromPanel(Rng2F32 root_rect, C_Panel *root, C_Panel *panel)
{
    M_Temp scratch = GetScratch(0, 0);
    
    // rjf: count ancestors
    U64 ancestor_count = 0;
    for(C_Panel *p = panel->parent; !C_PanelIsNil(p); p = p->parent)
    {
        ancestor_count += 1;
    }
    
    // rjf: gather ancestors
    C_Panel **ancestors = PushArrayZero(scratch.arena, C_Panel *, ancestor_count);
    {
        U64 ancestor_idx = 0;
        for(C_Panel *p = panel->parent; !C_PanelIsNil(p); p = p->parent)
        {
            ancestors[ancestor_idx] = p;
            ancestor_idx += 1;
        }
    }
    
    // rjf: go from highest ancestor => panel and calculate rect
    Rng2F32 parent_rect = root_rect;
    for(S64 ancestor_idx = (S64)ancestor_count-1;
        0 <= ancestor_idx && ancestor_idx < ancestor_count;
        ancestor_idx -= 1)
    {
        C_Panel *ancestor = ancestors[ancestor_idx];
        C_Panel *parent = ancestor->parent;
        if(!C_PanelIsNil(parent))
        {
            parent_rect = C_RectFromPanelChild(parent_rect, parent, ancestor);
        }
    }
    
    // rjf: calculate final rect
    Rng2F32 rect = C_RectFromPanelChild(parent_rect, panel->parent, panel);
    
    ReleaseScratch(scratch);
    return rect;
}

engine_function void
C_PanelFillInput(C_Panel *panel, String8 string)
{
    if(string.size > sizeof(panel->input_buffer))
    {
        string.size = sizeof(panel->input_buffer);
    }
    MemoryCopy(panel->input_buffer, string.str, string.size);
    panel->input.size = string.size;
    panel->cursor = panel->mark = TE_PointMake(1, panel->input.size+1);
    panel->input_is_focused = 0;
}

engine_function void
C_PanelError(C_Panel *panel, String8 string)
{
    if(string.size > sizeof(panel->error_buffer))
    {
        string.size = sizeof(panel->error_buffer);
    }
    MemoryCopy(panel->error_buffer, string.str, string.size);
    panel->error.size = string.size;
    panel->error_t = 1;
}

engine_function void
C_PanelErrorF(C_Panel *panel, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8F(scratch.arena, fmt, args);
    va_end(args);
    C_PanelError(panel, string);
    ReleaseScratch(scratch);
}

engine_function C_Handle
C_HandleFromPanel(C_Panel *panel)
{
    C_Handle h = {0};
    h.u64[0] = (U64)panel;
    h.u64[1] = panel->generation;
    return h;
}

engine_function C_Panel *
C_PanelFromHandle(C_Handle handle)
{
    C_Panel *panel = (C_Panel *)handle.u64[0];
    if(panel == 0 || panel->generation != handle.u64[1])
    {
        panel = &c_g_nil_panel;
    }
    return panel;
}

engine_function C_Entity *
C_EntityFromCommandString(String8 command)
{
    M_Temp scratch = GetScratch(0, 0);
    CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, command);
    String8 entity_id_str = cmd_node->first->string;
    C_Entity *result = C_EntityFromIDString(entity_id_str);
    ReleaseScratch(scratch);
    return result;
}

engine_function void
C_PanelInsertView(C_Panel *panel, C_View *prev_view, C_View *view)
{
    DLLInsert_NPZ(panel->first_view, panel->last_view, prev_view, view, next, prev, C_ViewIsNil, C_ViewSetNil);
    panel->view_count += 1;
}

engine_function void
C_PanelRemoveView(C_Panel *panel, C_View *view)
{
    DLLRemove_NPZ(panel->first_view, panel->last_view, view, next, prev, C_ViewIsNil, C_ViewSetNil);
    if(panel->selected_view == view)
    {
        C_View *fallback = C_ViewIsNil(view->prev) ? view->next : view->prev;
        panel->selected_view = fallback;
    }
    panel->view_count -= 1;
}

////////////////////////////////
//~ rjf: Entities

//- rjf: type engine_functions

engine_function B32
C_EntityIsNil(C_Entity *entity)
{
    return entity == 0 || entity == &c_g_nil_entity;
}

engine_function C_Handle
C_HandleFromEntity(C_Entity *entity)
{
    C_Handle handle = {0};
    handle.u64[0] = (U64)entity;
    handle.u64[1] = entity->generation;
    return handle;
}

engine_function C_Entity *
C_EntityFromHandle(C_Handle handle)
{
    C_Entity *entity = (C_Entity *)handle.u64[0];
    if((!C_EntityIsNil(entity) && entity->generation != handle.u64[1]) || C_EntityIsNil(entity))
    {
        entity = &c_g_nil_entity;
    }
    return entity;
}

//- rjf: allocation & tree building engine_functions

engine_function C_Entity *
C_EntityAlloc(C_Entity *parent, C_EntityKind kind)
{
    // rjf: allocate entity
    C_Entity *entity = c_state->free_entity;
    if(C_EntityIsNil(entity))
    {
        entity = PushArrayZero(c_state->arena, C_Entity, 1);
    }
    else
    {
        StackPop(c_state->free_entity);
        U64 generation = entity->generation;
        MemoryZeroStruct(entity);
        entity->generation = generation + 1;
    }
    
    // rjf: generate ID
    c_state->entity_id_gen += 1;
    entity->id = c_state->entity_id_gen;
    
    // rjf: set parent to root, if no parent was passed
    if(C_EntityIsNil(parent) && !C_EntityIsNil(c_state->root_entity))
    {
        parent = c_state->root_entity;
    }
    
    // rjf: make this the root if we have none
    if(C_EntityIsNil(c_state->root_entity))
    {
        c_state->root_entity = entity;
    }
    
    // rjf: hook into tree links
    if(!C_EntityIsNil(parent))
    {
        DLLInsert_NPZ(parent->first, parent->last, parent->last, entity, next, prev, C_EntityIsNil, C_EntitySetNil);
        entity->parent = parent;
    }
    
    // rjf: fill
    {
        entity->kind = kind;
    }
    
    return entity;
}

engine_function void
C_EntityRelease(C_Entity *entity)
{
    C_Entity *parent = entity->parent;
    if(!C_EntityIsNil(parent))
    {
        DLLRemove_NPZ(parent->first, parent->last, entity, next, prev, C_EntityIsNil, C_EntitySetNil);
    }
    for(C_Entity *child = entity->first, *next = 0; !C_EntityIsNil(child); child = next)
    {
        next = child->next;
        C_EntityRelease(child);
    }
    if(!C_EntityIsNil(entity))
    {
        StackPush(c_state->free_entity, entity);
        entity->generation += 1;
        if(entity->name_arena != 0)
        {
            M_ArenaRelease(entity->name_arena);
        }
    }
}

engine_function C_Entity *
C_EntityChangeParent(C_Entity *entity, C_Entity *new_parent)
{
    C_Entity *old_parent = entity->parent;
    DLLRemove_NPZ(old_parent->first, old_parent->last, entity, next, prev, C_EntityIsNil, C_EntitySetNil);
    DLLPushBack_NPZ(new_parent->first, new_parent->last, entity, next, prev, C_EntityIsNil, C_EntitySetNil);
    entity->parent = new_parent;
    return entity;
}

//- rjf: equipment

engine_function M_Arena *
C_EntityEquipOrClearNameArena(C_Entity *entity)
{
    if(entity->name_arena == 0)
    {
        entity->name_arena = M_ArenaAlloc(Kilobytes(1));
    }
    else
    {
        M_ArenaClear(entity->name_arena);
    }
    return entity->name_arena;
}

engine_function void
C_EntityEquipName(C_Entity *entity, String8 string)
{
    M_Arena *arena = C_EntityEquipOrClearNameArena(entity);
    entity->name = PushStr8Copy(entity->name_arena, string);
}

engine_function void
C_EntityEquipTextPoint(C_Entity *entity, TE_Point point)
{
    entity->txt_point = point;
    entity->flags |= C_EntityFlag_HasTextPoint;
}

//- rjf: introspection

engine_function C_Entity *
C_EntityChildFromKindAndString(C_Entity *parent, C_EntityKind kind, String8 string, MatchFlags flags)
{
    C_Entity *result = &c_g_nil_entity;
    for(C_Entity *child = parent->first; !C_EntityIsNil(child); child = child->next)
    {
        if(child->kind == kind && Str8Match(child->name, string, flags))
        {
            result = child;
            break;
        }
    }
    return result;
}

engine_function String8
C_FullPathFromEntity(M_Arena *arena, C_Entity *entity)
{
    M_Temp scratch = GetScratch(&arena, 1);
    String8List strs = {0};
    for(C_Entity *p = entity->parent; !C_EntityIsNil(p) && p->kind == C_EntityKind_Folder; p = p->parent)
    {
        Str8ListPushFront(scratch.arena, &strs, p->name);
    }
    Str8ListPush(scratch.arena, &strs, entity->name);
    StringJoin join = {0};
    join.sep = Str8Lit("/");
    String8 result = Str8ListJoin(arena, strs, &join);
    ReleaseScratch(scratch);
    return result;
}

//- rjf: recursion iterators

engine_function C_EntityRec
C_EntityRecDF(C_Entity *entity, MemberOffset child_off, MemberOffset sib_off)
{
    C_EntityRec result = {0};
    if(!C_EntityIsNil(MemberFromOff(entity, C_Entity *, child_off)))
    {
        result.next = MemberFromOff(entity, C_Entity *, child_off);
        result.push_count += 1;
    }
    else for(C_Entity *p = entity; !C_EntityIsNil(p); p = p->parent)
    {
        if(!C_EntityIsNil(MemberFromOff(p, C_Entity *, sib_off)))
        {
            result.next = MemberFromOff(p, C_Entity *, sib_off);
            break;
        }
        result.pop_count += 1;
    }
    return result;
}

////////////////////////////////
//~ rjf: Entity Construction Helpers

engine_function C_Entity *
C_OpenPath(String8 path)
{
    C_Entity *entity = &c_g_nil_entity;
    M_Temp scratch = GetScratch(0, 0);
    String8 full_path = OS_NormalizedPathFromString(scratch.arena, path);
    
    // rjf: break full path down into chain of directories
    String8List directories = {0};
    {
        String8 slash_splits[] =
        {
            Str8LitComp("/"),
            Str8LitComp("\\"),
        };
        directories = StrSplit8(scratch.arena, full_path, ArrayCount(slash_splits), slash_splits);
    }
    
    // rjf: get or construct each directory, and retrieve directory parent
    C_Entity *parent = C_RootEntity();
    {
        for(String8Node *n = directories.first; n != 0; n = n->next)
        {
            C_Entity *dir_entity = C_EntityChildFromKindAndString(parent, C_EntityKind_Folder, n->string, MatchFlag_CaseInsensitive);
            if(C_EntityIsNil(dir_entity))
            {
                dir_entity = C_EntityAlloc(parent, C_EntityKind_Folder);
                C_EntityEquipName(dir_entity, n->string);
            }
            parent = dir_entity;
        }
    }
    
    return parent;
}

engine_function C_Entity *
C_OpenFile(String8 path)
{
    C_Entity *entity = &c_g_nil_entity;
    M_Temp scratch = GetScratch(0, 0);
    String8 full_path = OS_NormalizedPathFromString(scratch.arena, path);
    
    // rjf: open path & get folder parent
    C_Entity *parent = C_OpenPath(Str8ChopLastSlash(full_path));
    
    // rjf: check if we already have this file
    C_Entity *existing_file = C_EntityChildFromKindAndString(parent, C_EntityKind_File, Str8SkipLastSlash(full_path), MatchFlag_CaseInsensitive);
    
    // rjf: make entity for file, if we don't have it.
    if(C_EntityIsNil(existing_file))
    {
        OS_FileLoadResult load_result = OS_LoadEntireFile(scratch.arena, full_path);
        String8 data = load_result.string;
        B32 good_load = load_result.success;
        if(good_load)
        {
            entity = C_EntityAlloc(parent, C_EntityKind_File);
            C_EntityEquipName(entity, Str8SkipLastSlash(full_path));
            // C_EntityEquipMutableText(entity, data);
        }
    }
    else
    {
        entity = existing_file;
    }
    
    ReleaseScratch(scratch);
    return entity;
}

engine_function C_Entity *
C_EntityEquipPathName(C_Entity *entity, String8 path)
{
    String8 folder = Str8ChopLastSlash(path);
    String8 filename = Str8SkipLastSlash(path);
    C_Entity *parent = C_OpenPath(folder);
    C_EntityChangeParent(entity, parent);
    C_EntityEquipName(entity, filename);
    return entity;
}

////////////////////////////////
//~ rjf: Plugins

per_thread C_Plugin *c_t_selected_plugin = 0;

engine_function void *
C_PluginTickGlobalStub(M_Arena *arena, void *state_ptr, OS_EventList *events, C_CmdList *cmds)
{
    return state_ptr;
}

engine_function void *
C_PluginTickViewStub(M_Arena *arena, void *state_ptr, APP_Window *window, OS_EventList *events, C_CmdList *cmds, Rng2F32 rect)
{
    return state_ptr;
}

engine_function void
C_PluginCloseViewStub(void *state_ptr)
{
}

engine_function C_Plugin *
C_PluginFromString(String8 string)
{
    C_Plugin *plugin = 0;
    U64 hash = C_HashFromString_CaseInsensitive(string);
    U64 slot = hash % c_state->plugin_table_size;
    for(C_Plugin *p = c_state->plugin_table[slot].first; p != 0; p = p->hash_next)
    {
        if(Str8Match(p->name, string, MatchFlag_CaseInsensitive))
        {
            plugin = p;
            break;
        }
    }
    return plugin;
}

engine_function void
C_SelectPlugin(C_Plugin *plugin)
{
    c_t_selected_plugin = plugin;
}

engine_function void
C_RegisterCommands(U64 count, C_CmdKindInfo *info)
{
    C_Plugin *plugin = c_t_selected_plugin;
    if(plugin != 0)
    {
        for(U64 idx = 0; idx < count; idx += 1)
        {
            C_CmdKindInfo i = info[idx];
            U64 hash = C_HashFromString_CaseInsensitive(i.name);
            U64 slot = hash % plugin->cmd_kind_node_table_size;
            C_PluginCmdKindNode *existing_node = 0;
            for(C_PluginCmdKindNode *n = plugin->cmd_kind_node_table[slot]; n != 0; n = n->hash_next)
            {
                if(Str8Match(n->name, i.name, MatchFlag_CaseInsensitive))
                {
                    existing_node = n;
                    break;
                }
            }
            if(existing_node == 0)
            {
                C_PluginCmdKindNode *n = PushArrayZero(plugin->arena, C_PluginCmdKindNode, 1);
                n->name = PushStr8Copy(plugin->arena, i.name);
                n->description = PushStr8Copy(plugin->arena, i.description);
                n->hash_next = plugin->cmd_kind_node_table[slot];
                plugin->cmd_kind_node_table[slot] = n;
            }
        }
    }
}

////////////////////////////////
//~ rjf: State Accessors

engine_function R_Handle
C_Logo(void)
{
    return c_state->logo;
}

engine_function C_Entity *
C_RootEntity(void)
{
    return c_state->root_entity;
}

engine_function C_Entity *
C_EntityFromID(C_EntityID id)
{
    C_Entity *result = &c_g_nil_entity;
    for(C_Entity *entity = C_RootEntity(); !C_EntityIsNil(entity); entity = C_EntityRecDF_Pre(entity).next)
    {
        if(entity->id == id)
        {
            result = entity;
            break;
        }
    }
    return result;
}

engine_function C_Entity *
C_EntityFromIDString(String8 string)
{
    C_Entity *result = &c_g_nil_entity;
    U64 dollar_sign_pos = FindSubstr8(string, Str8Lit("$"), 0, 0);
    if(dollar_sign_pos < string.size)
    {
        String8 numeric_part = Str8Skip(string, dollar_sign_pos+1);
        C_EntityID id = U64FromStr8(numeric_part, 10);
        result = C_EntityFromID(id);
    }
    return result;
}

engine_function C_Entity *
C_EntityFromKindAndString(C_EntityKind kind, String8 string)
{
    C_Entity *result = &c_g_nil_entity;
    for(C_Entity *entity = C_RootEntity(); !C_EntityIsNil(entity); entity = C_EntityRecDF_Pre(entity).next)
    {
        if(entity->kind == kind && Str8Match(entity->name, string, 0))
        {
            result = entity;
            break;
        }
    }
    return result;
}

////////////////////////////////
//~ rjf: Jobs

engine_function void
C_WorkerPushData(C_Worker *worker, U64 size, void *data)
{
    U64 space_taken_in_worker = worker->ring_write_pos - worker->ring_read_pos;
    U64 space_left_in_worker = worker->ring_size - space_taken_in_worker;
    Assert(space_left_in_worker >= size);
    U64 bytes_left_to_write = size;
    void *write = data;
    for(;bytes_left_to_write > 0;)
    {
        U64 write_off = worker->ring_write_pos % worker->ring_size;
        U64 write_size = ClampTop(bytes_left_to_write, worker->ring_size - write_off);
        MemoryCopy(worker->ring + write_off, write, write_size);
        bytes_left_to_write -= write_size;
        write = (U8 *)write + write_size;
    }
    AtomicAdd64(&worker->ring_write_pos, size);
}

engine_function C_Handle
C_PushJob(String8 params, C_JobFunction *func)
{
    // TODO(rjf): Better worker selection strategy:
    // find a worker with the smallest difference between
    // ring_read_pos and ring_write_pos
    //
    // ... or maybe that is stupid, because there is not necessarily a
    // correllation between parameters size & computation time? Or maybe
    // there is?
    //
    // but in any case, we should calculate some "cost factor" and minimize
    // that to pick a worker.
    
    //- rjf: try to find a worker with enough space for this job 
    C_Worker *found_worker = 0;
    for(U64 worker_idx = 0; worker_idx < c_state->worker_count; worker_idx += 1)
    {
        C_Worker *worker = c_state->workers + worker_idx;
        U64 bytes_taken_in_worker = worker->ring_write_pos - worker->ring_read_pos;
        U64 bytes_available_in_worker = worker->ring_size - bytes_taken_in_worker;
        Assert(bytes_taken_in_worker <= worker->ring_size);
        if(bytes_available_in_worker >= params.size + sizeof(C_Job))
        {
            found_worker = worker;
            if(bytes_taken_in_worker == 0)
            {
                break;
            }
        }
    }
    
    //- rjf: if we did not find a worker, then we need to wait on one that seems
    // like it won't take forever to complete (but for now let's do a crappy
    // strategy)
    if(found_worker == 0)
    {
        for(U64 worker_idx = 0; worker_idx < c_state->worker_count; worker_idx += 1)
        {
            C_Worker *worker = c_state->workers + worker_idx;
            OS_SemaphoreWait(worker->job_done_semaphore, U32Max);
            U64 bytes_taken_in_worker = worker->ring_write_pos - worker->ring_read_pos;
            U64 bytes_available_in_worker = worker->ring_size - bytes_taken_in_worker;
            if(bytes_available_in_worker >= params.size + sizeof(C_Job))
            {
                found_worker = worker;
                break;
            }
        }
    }
    
    //- rjf: push the job if we found a worker (which we better have, because
    // the above code should have a fallback)
    if(found_worker != 0)
    {
        C_Job job = {params.size, func};
        C_WorkerPushStruct(found_worker, &job);
        C_WorkerPushStr8Copy(found_worker, params);
        OS_SemaphoreSignal(found_worker->jobs_present_semaphore);
    }
    
    C_Handle handle = {0};
    return handle;
}

engine_function void
C_WorkerThreadFunction(void *p)
{
    C_Worker *worker = (C_Worker *)p;
    for(;!c_g_quit_signal;)
    {
        OS_SemaphoreSignal(worker->job_done_semaphore);
        OS_SemaphoreWait(worker->jobs_present_semaphore, U32Max);
        M_Temp scratch = GetScratch(0, 0);
        if(worker->ring_write_pos >= worker->ring_read_pos + sizeof(C_Job))
        {
            U64 read_pos = worker->ring_read_pos;
            U64 start_read_pos = read_pos;
            C_Job *job = (C_Job *)(worker->ring + read_pos);
            read_pos += sizeof(*job);
            String8List params_strs = {0};
            U64 params_bytes_left = job->params_size;
            for(;read_pos < start_read_pos + job->params_size + sizeof(C_Job);)
            {
                U64 read_off = read_pos % worker->ring_size;
                U64 read_size = ClampTop(worker->ring_size - read_off, params_bytes_left);
                String8 param_str = Str8(worker->ring + read_off, read_size);
                Str8ListPush(scratch.arena, &params_strs, param_str);
                read_pos += read_size;
                params_bytes_left -= read_size;
            }
            String8 params = Str8ListJoin(scratch.arena, params_strs, 0);
            job->func((void *)params.str);
            AtomicAdd64(&worker->ring_read_pos, job->params_size + sizeof(C_Job));
        }
        ReleaseScratch(scratch);
    }
}

////////////////////////////////
//~ rjf: Main Hooks

engine_function void
C_Init(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(8));
    c_state = PushArrayZero(arena, C_State, 1);
    c_state->arena = arena;
    c_state->root_entity = C_EntityAlloc(&c_g_nil_entity, C_EntityKind_Root);
    c_state->plugin_table_size = 256;
    c_state->plugin_table = PushArrayZero(arena, C_PluginSlot, c_state->plugin_table_size);
    c_state->logo = APP_LoadTexture(Str8Lit("logo.png"));
    
    U64 num_logical_cores = OS_LogicalProcessorCount();
    c_state->worker_count = ClampBot(2, num_logical_cores-1);
    c_state->workers = PushArrayZero(arena, C_Worker, c_state->worker_count);
    for(U64 worker_idx = 0; worker_idx < c_state->worker_count; worker_idx += 1)
    {
        C_Worker *worker = &c_state->workers[worker_idx];
        worker->ring_size = Megabytes(16);
        worker->ring = PushArray(arena, U8, worker->ring_size);
        worker->jobs_present_semaphore = OS_SemaphoreAlloc(U32Max);
        worker->job_done_semaphore = OS_SemaphoreAlloc(1);
        worker->thread = OS_ThreadStart(worker, C_WorkerThreadFunction);
    }
}

engine_function void
C_BeginFrame(void)
{
    typedef struct TimestampNode TimestampNode;
    struct TimestampNode
    {
        TimestampNode *next;
        OS_Timestamp timestamp;
    };
    
    M_Temp scratch = GetScratch(0, 0);
    String8 plugin_search_path = Str8Lit("./plugins/");
    
    //- rjf: get list of plugin files
    String8List plugin_filenames = {0};
    TimestampNode *first_timestamp = 0;
    TimestampNode *last_timestamp = 0;
    {
        OS_FileIter *it = OS_FileIterBegin(scratch.arena, plugin_search_path);
        for(OS_FileInfo info = {0}; OS_FileIterNext(scratch.arena, it, &info);)
        {
            String8 extension = Str8SkipLastPeriod(info.name);
            if(Str8Match(extension, Str8Lit("dll"), MatchFlag_CaseInsensitive) ||
               Str8Match(extension, Str8Lit("so"), MatchFlag_CaseInsensitive) ||
               Str8Match(extension, Str8Lit("dylib"), MatchFlag_CaseInsensitive))
            {
                Str8ListPush(scratch.arena, &plugin_filenames, info.name);
                TimestampNode *timestamp = PushArrayZero(scratch.arena, TimestampNode, 1);
                timestamp->timestamp = info.attributes.last_modified;
                QueuePush(first_timestamp, last_timestamp, timestamp);
            }
        }
        OS_FileIterEnd(it);
    }
    
    //- rjf: build list of plugins & fill new retained slots in the plugin table
    C_Plugin *first_plugin = 0;
    C_Plugin *last_plugin = 0;
    {
        String8Node *n = plugin_filenames.first;
        TimestampNode *ts_n = first_timestamp;
        for(;n != 0 && ts_n != 0; n = n->next, ts_n = ts_n->next)
        {
            // rjf: get hash/slot
            String8 plugin_name = Str8ChopLastPeriod(n->string);
            
            // rjf: try to map to an existing plugin
            C_Plugin *plugin = C_PluginFromString(plugin_name);
            
            // rjf: if we didn't get a plugin, then we need to set one up
            B32 need_reload = 0;
            if(plugin == 0)
            {
                U64 hash = C_HashFromString_CaseInsensitive(plugin_name);
                U64 slot = hash % c_state->plugin_table_size;
                plugin = c_state->free_plugin;
                if(plugin != 0)
                {
                    c_state->free_plugin = c_state->free_plugin->next;
                    MemoryZeroStruct(plugin);
                }
                else
                {
                    plugin = PushArrayZero(c_state->arena, C_Plugin, 1);
                }
                DLLPushBack_NPZ(c_state->plugin_table[slot].first, c_state->plugin_table[slot].last, plugin, hash_next, hash_prev, CheckNull, SetNull);
                plugin->arena = M_ArenaAlloc(Gigabytes(8));
                plugin->name = PushStr8Copy(plugin->arena, plugin_name);
                plugin->cmd_kind_node_table_size = 256;
                plugin->cmd_kind_node_table = PushArrayZero(plugin->arena, C_PluginCmdKindNode *, plugin->cmd_kind_node_table_size);
                need_reload = 1;
            }
            
            // rjf: if we *did* get the plugin, and the timestamp is out-of-date, then
            // we need to reload
            need_reload = (need_reload || (plugin != 0 && plugin->last_modified_time != ts_n->timestamp));
            
            // rjf: reload if needed
            if(need_reload)
            {
                if(!OS_HandleMatch(OS_HandleZero(), plugin->library))
                {
                    OS_LibraryClose(plugin->library);
                }
                String8 plugin_path = PushStr8F(scratch.arena, "%S%S", plugin_search_path, n->string);
                String8 plugin_loaded_path = PushStr8F(scratch.arena, "client_loaded__%S", n->string);
                B32 copy_success = OS_CopyFile(plugin_path, plugin_loaded_path);
                plugin->TickGlobal = 0;
                plugin->TickView = 0;
                plugin->CloseView = 0;
                if(copy_success)
                {
                    plugin->library = OS_LibraryOpen(plugin_loaded_path);
                    plugin->TickGlobal = (C_PluginTickGlobalFunction *)OS_LibraryLoadFunction(plugin->library, Str8Lit("TickGlobal"));
                    plugin->TickView = (C_PluginTickViewFunction *)OS_LibraryLoadFunction(plugin->library, Str8Lit("TickView"));
                    plugin->CloseView = (C_PluginCloseViewFunction *)OS_LibraryLoadFunction(plugin->library, Str8Lit("CloseView"));
                }
                if((plugin->TickGlobal != 0 || plugin->TickView != 0 || plugin->CloseView != 0) && copy_success)
                {
                    plugin->last_modified_time = ts_n->timestamp;
                }
                if(plugin->TickGlobal == 0)
                {
                    plugin->TickGlobal = C_PluginTickGlobalStub;
                }
                if(plugin->TickView == 0)
                {
                    plugin->TickView = C_PluginTickViewStub;
                }
                if(plugin->CloseView == 0)
                {
                    plugin->CloseView = C_PluginCloseViewStub;
                }
            }
            
            // rjf: push plugin into linear list
            if(plugin != 0)
            {
                QueuePush(first_plugin, last_plugin, plugin);
            }
        }
    }
    
    ReleaseScratch(scratch);
}

engine_function void
C_EndFrame(void)
{
    c_state->frame_index += 1;
}

engine_function void
C_Quit(void)
{
    c_g_quit_signal = 1;
    for(U64 worker_idx = 0; worker_idx < c_state->worker_count; worker_idx += 1)
    {
        OS_SemaphoreSignal(c_state->workers[worker_idx].jobs_present_semaphore);
    }
}
