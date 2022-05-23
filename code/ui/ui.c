// TODO(rjf): Currently, if a row or column are used, and their children exceed
// the set sizes of the row or column, then the row/column will stay the same
// size, and the children will overflow. That is normally okay, but when doing
// children size calculations, it will break unless those rows/columns are
// explicitly marked as being sized by a children-sum. This is sort of mislead-
// -ing, so maybe we can expand parents that do not allow overflow, if their
// children overflow?

////////////////////////////////
//~ rjf: Globals

#if ENGINE
read_only UI_Box ui_g_nil_box =
{
    &ui_g_nil_box,
    &ui_g_nil_box,
    &ui_g_nil_box,
    &ui_g_nil_box,
    &ui_g_nil_box,
};
#endif
per_thread UI_State *ui_state = 0;

////////////////////////////////
//~ rjf: Sizes

engine_function UI_Size
UI_SizeMake(UI_SizeKind kind, F32 value, F32 strictness)
{
    UI_Size size = {kind};
    size.value = value;
    size.strictness = strictness;
    return size;
}

////////////////////////////////
//~ rjf: Keys

engine_function U64
UI_HashFromString(U64 seed, String8 string)
{
    U64 result = seed;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}

engine_function UI_Key
UI_KeyNull(void)
{
    UI_Key result = {0};
    return result;
}

engine_function String8
UI_HashPartFromBoxString(String8 string)
{
    String8 result = string;
    
    // rjf: look for ### patterns, which can replace the entirety of the part of
    // the string that is hashed.
    U64 hash_replace_signifier_pos = FindSubstr8(string, Str8Lit("###"), 0, MatchFlag_FindLast);
    if(hash_replace_signifier_pos < string.size)
    {
        result = Str8Skip(string, hash_replace_signifier_pos);
    }
    
    return result;
}

engine_function String8
UI_DisplayStringFromBox(UI_Box *box)
{
    String8 string = box->string;
    if(!(box->flags & UI_BoxFlag_UseFullString))
    {
        U64 dbl_hash_pos = FindSubstr8(string, Str8Lit("##"), 0, 0);
        string.size = dbl_hash_pos;
    }
    return string;
}

engine_function UI_Key
UI_KeyFromString(UI_Key seed_key, String8 string)
{
    UI_Key result = {0};
    if(string.size != 0)
    {
        String8 hash_part = UI_HashPartFromBoxString(string);
        result.u64[0] = UI_HashFromString(seed_key.u64[0], hash_part);
    }
    return result;
}

engine_function B32
UI_KeyMatch(UI_Key a, UI_Key b)
{
    return a.u64[0] == b.u64[0];
}

////////////////////////////////
//~ rjf: Boxes

engine_function B32
UI_BoxIsNil(UI_Box *box)
{
    return box == 0 || box == &ui_g_nil_box;
}

engine_function UI_BoxRec
UI_BoxRecDF(UI_Box *box, U64 sib_member_off, U64 child_member_off)
{
    UI_BoxRec result = {0};
    if(!UI_BoxIsNil(*MemberFromOffset(UI_Box **, box, child_member_off)))
    {
        result.next = *MemberFromOffset(UI_Box **, box, child_member_off);
        result.push_count = 1;
    }
    else if(!UI_BoxIsNil(*MemberFromOffset(UI_Box **, box, sib_member_off)))
    {
        result.next = *MemberFromOffset(UI_Box **, box, sib_member_off);
    }
    else
    {
        UI_Box *uncle = &ui_g_nil_box;
        for(UI_Box *p = box->parent; !UI_BoxIsNil(p); p = p->parent)
        {
            result.pop_count += 1;
            if(!UI_BoxIsNil(*MemberFromOffset(UI_Box **, p, sib_member_off)))
            {
                uncle = *MemberFromOffset(UI_Box **, p, sib_member_off);
                break;
            }
        }
        result.next = uncle;
    }
    return result;
}

engine_function UI_BoxRec
UI_BoxRecDF_Pre(UI_Box *box)
{
    return UI_BoxRecDF(box, OffsetOf(UI_Box, next), OffsetOf(UI_Box, first));
}

engine_function UI_BoxRec
UI_BoxRecDF_Post(UI_Box *box)
{
    return UI_BoxRecDF(box, OffsetOf(UI_Box, prev), OffsetOf(UI_Box, last));
}

////////////////////////////////
//~ rjf: Global (Not Per-State) Queries

global B32 ui_g_drag_active = 0;

engine_function B32
UI_DragIsActive(void)
{
    return ui_g_drag_active;
}

////////////////////////////////
//~ rjf: State Building / Selecting

engine_function UI_State *
UI_StateAlloc(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(64));
    UI_State *ui = PushArrayZero(arena, UI_State, 1);
    ui->arena = arena;
    ui->frame_arenas[0] = M_ArenaAlloc(Gigabytes(1));
    ui->frame_arenas[1] = M_ArenaAlloc(Gigabytes(1));
    ui->box_table_size = 4096;
    ui->box_table = PushArrayZero(arena, UI_BoxHashSlot, ui->box_table_size);
    return ui;
}

engine_function void
UI_StateRelease(UI_State *state)
{
    M_ArenaRelease(state->arena);
}

engine_function UI_State *
UI_GetSelectedState(void)
{
    return ui_state;
}

engine_function UI_Box *
UI_RootFromState(UI_State *state)
{
    return state->root;
}

////////////////////////////////
//~ rjf: Implicit State Accessors

engine_function M_Arena *
UI_FrameArena(void)
{
    M_Arena *result = ui_state->frame_arenas[ui_state->frame_index%ArrayCount(ui_state->frame_arenas)];
    return result;
}

engine_function OS_Handle
UI_Window(void)
{
    return ui_state->window;
}

engine_function OS_EventList *
UI_Events(void)
{
    return ui_state->events;
}

engine_function F32
UI_CaretBlinkT(void)
{
    return ui_state->caret_blink_t;
}

engine_function void
UI_ResetCaretBlinkT(void)
{
    ui_state->caret_blink_t = 0;
}

engine_function void
UI_StoreDragData_Vec2F32(Vec2F32 v)
{
    F32 *storage = (F32 *)&ui_state->drag_data[0];
    storage[0] = v.x;
    storage[1] = v.y;
}

engine_function Vec2F32
UI_GetDragData_Vec2F32(void)
{
    F32 *storage = (F32 *)&ui_state->drag_data[0];
    Vec2F32 result = V2F32(storage[0], storage[1]);
    return result;
}

engine_function UI_Key
UI_HotKey(void)
{
    return ui_state->hot_box_key;
}

////////////////////////////////
//~ rjf: Top-Level Building API

engine_function void
UI_BeginBuild(UI_State *state, OS_EventList *events, OS_Handle window, F32 animation_dt)
{
    ui_state = state;
    
    //- rjf: reset per-frame ui state
    M_ArenaClear(UI_FrameArena());
    ui_state->events = events;
    ui_state->window = window;
    ui_state->parent_stack_size = 0;
    ui_state->parent_stack_active = &ui_g_nil_box;
    ui_state->view_off_storage_stack_size = 0;
    ui_state->view_off_storage_stack_active = 0;
    ui_state->root = &ui_g_nil_box;
    ui_state->animation_dt = animation_dt;
    UI_ZeroAllStacks(ui_state);
    
    //- rjf: make box for this window
    Rng2F32 window_rect = OS_ClientRectFromWindow(window);
    Vec2F32 window_rect_size = Dim2F32(window_rect);
    UI_PushFixedWidth(window_rect_size.x);
    UI_PushFixedHeight(window_rect_size.y);
    UI_Box *root = UI_BoxMake(0, PushStr8F(UI_FrameArena(), "###%" PRIx64 "", window.u64[0]));
    UI_BoxEquipChildLayoutAxis(root, Axis2_X);
    ui_state->parent_stack_size = 0;
    ui_state->parent_stack_active = root;
    ui_state->root = root;
    UI_PopFixedWidth();
    UI_PopFixedHeight();
    
    //- rjf: setup parent box for tooltip
    Vec2F32 mouse = OS_MouseFromWindow(window);
    UI_FixedX(mouse.x+15.f) UI_FixedY(mouse.y+15.f) UI_PrefWidth(UI_ChildrenSum(1.f)) UI_PrefHeight(UI_ChildrenSum(1.f))
    {
        ui_state->tooltip_root = UI_BoxMake(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, PushStr8F(UI_FrameArena(), "###tooltip_%" PRIx64 "", window.u64[0]));
    }
    
    //- rjf: push initial stack values
    UI_PushPrefWidth(UI_Pixels(150.f, 0.4f));
    UI_PushPrefHeight(UI_Pixels(22.f, 1.f));
    UI_PushBackgroundColor(V4F32(0.15f, 0.15f, 0.15f, 0.8f));
    UI_PushTextColor(V4F32(1, 1, 1, 0.8f));
    UI_PushBorderColor(V4F32(1, 1, 1, 0.2f));
    UI_PushCornerRadius(3.f);
    
    //- rjf: reset hot if we don't have an active widget
    if(UI_KeyMatch(ui_state->active_box_key, UI_KeyNull()))
    {
        ui_state->hot_box_key = UI_KeyNull();
    }
    
    //- rjf: set global drag state
    {
        B32 active = !UI_KeyMatch(ui_state->active_box_key, UI_KeyNull());
        if(active)
        {
            ui_state->drag_started = 1;
        }
        if(ui_state->drag_started)
        {
            ui_g_drag_active = active;
        }
        if(!active && ui_state->drag_started)
        {
            ui_state->drag_started = 0;
        }
    }
}

engine_function void
UI_EndBuild(void)
{
    //- rjf: prune untouched or transient widgets in the cache
    {
        for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx += 1)
        {
            for(UI_Box *box = ui_state->box_table[slot_idx].hash_first, *next = 0;
                !UI_BoxIsNil(box);
                box = next)
            {
                next = box->hash_next;
                if(box->last_touched_frame_index < ui_state->frame_index ||
                   UI_KeyMatch(box->key, UI_KeyNull()))
                {
                    DLLRemove_NPZ(ui_state->box_table[slot_idx].hash_first, ui_state->box_table[slot_idx].hash_last, box, hash_next, hash_prev, UI_BoxIsNil, UI_BoxSetNil);
                    StackPush_N(ui_state->first_free_box, box, next);
                }
            }
        }
    }
    
    //- rjf: solve for sizes & position all boxes, on both axes
    for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
    {
        UI_LayoutStandalone_InPlaceRecursive(ui_state->root, axis);
        UI_LayoutUpwardsDependent_InPlaceRecursive(ui_state->root, axis);
        UI_LayoutDownwardsDependent_InPlaceRecursive(ui_state->root, axis);
        UI_LayoutEnforceConstraints_InPlaceRecursive(ui_state->root, axis);
        UI_LayoutPosition_InPlaceRecursive(ui_state->root, axis);
    }
    
    //- rjf: animate
    {
        ui_state->caret_blink_t += ui_state->animation_dt;
        for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx += 1)
        {
            for(UI_Box *box = ui_state->box_table[slot_idx].hash_first;
                !UI_BoxIsNil(box);
                box = box->hash_next)
            {
                
                // rjf: animate interaction transition states
                box->hot_t    += ui_state->animation_dt * 28.f * (((F32)UI_KeyMatch(box->key, ui_state->hot_box_key)) - box->hot_t);
                box->active_t += ui_state->animation_dt * 28.f * (((F32)UI_KeyMatch(box->key, ui_state->active_box_key)) - box->active_t);
                
                // rjf: animate positions
                {
                    box->fixed_position_animated.x += ui_state->animation_dt * 28.f * (box->fixed_position.x - box->fixed_position_animated.x);
                    box->fixed_position_animated.y += ui_state->animation_dt * 28.f * (box->fixed_position.y - box->fixed_position_animated.y);
                }
                
                // rjf: animate view offset
                Vec2F32 max_target_view_off =
                {
                    ClampBot(0, box->view_bounds.x - box->fixed_size.x),
                    ClampBot(0, box->view_bounds.y - box->fixed_size.y),
                };
                box->target_view_off.x = Clamp(0, box->target_view_off.x, max_target_view_off.x);
                box->target_view_off.y = Clamp(0, box->target_view_off.y, max_target_view_off.y);
                box->target_view_off.x = (F32)(S32)box->target_view_off.x;
                box->target_view_off.y = (F32)(S32)box->target_view_off.y;
                box->view_off.x += ui_state->animation_dt * 28.f * (box->target_view_off.x - box->view_off.x);
                box->view_off.y += ui_state->animation_dt * 28.f * (box->target_view_off.y - box->view_off.y);
                if(box->view_off_storage)
                {
                    *box->view_off_storage = box->target_view_off;
                }
            }
        }
    }
    
    //- rjf: hover cursor
    if(OS_WindowIsFocused(ui_state->window))
    {
        UI_Box *hot = UI_BoxFromKey(ui_state->hot_box_key);
        UI_Box *active = UI_BoxFromKey(ui_state->active_box_key);
        UI_Box *box = UI_BoxIsNil(active) ? hot : active;
        APP_Cursor(box->hover_cursor);
    }
    
    ui_state->frame_index += 1;
}

engine_function void
UI_LayoutStandalone_InPlaceRecursive(UI_Box *root, Axis2 axis)
{
    switch(root->pref_size[axis].kind)
    {
        case UI_SizeKind_Pixels:
        {
            root->fixed_size.v[axis] = root->pref_size[axis].value;
        }break;
        
        case UI_SizeKind_TextContent:
        {
            String8 display_string = UI_DisplayStringFromBox(root);
            F32 padding = root->pref_size[axis].value;
            F32 text_size = R_AdvanceFromString(root->font, display_string).v[axis];
            root->fixed_size.v[axis] = padding + text_size;
        }break;
    }
    
    //- rjf: recurse
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        UI_LayoutStandalone_InPlaceRecursive(child, axis);
    }
}

engine_function void
UI_LayoutUpwardsDependent_InPlaceRecursive(UI_Box *root, Axis2 axis)
{
    //- rjf: solve for all kinds that are upwards-dependent
    switch(root->pref_size[axis].kind)
    {
        default: break;
        
        // rjf: if root has a parent percentage, figure out its size
        case UI_SizeKind_ParentPct:
        {
            // rjf: find parent that has a fixed size
            UI_Box *fixed_parent = &ui_g_nil_box;
            for(UI_Box *p = root->parent; !UI_BoxIsNil(p); p = p->parent)
            {
                if(p->flags & (UI_BoxFlag_FixedWidth<<axis))
                {
                    fixed_parent = p;
                    break;
                }
            }
            
            // rjf: figure out root's size on this axis
            F32 size = fixed_parent->fixed_size.v[axis] * root->pref_size[axis].value;
            
            // rjf: mutate root to have this size, set as fixed
            root->fixed_size.v[axis] = size;
            root->flags |= UI_BoxFlag_FixedWidth;
        }break;
    }
    
    //- rjf: recurse
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        UI_LayoutUpwardsDependent_InPlaceRecursive(child, axis);
    }
}

engine_function void
UI_LayoutDownwardsDependent_InPlaceRecursive(UI_Box *root, Axis2 axis)
{
    //- rjf: recurse first. we may depend on children that have
    // the same property
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        UI_LayoutDownwardsDependent_InPlaceRecursive(child, axis);
    }
    
    //- rjf: solve for all kinds that are downwards-dependent
    switch(root->pref_size[axis].kind)
    {
        default: break;
        
        // rjf: sum children
        case UI_SizeKind_ChildrenSum:
        {
            F32 sum = 0;
            for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
            {
                if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
                {
                    if(axis == root->child_layout_axis)
                    {
                        sum += child->fixed_size.v[axis];
                    }
                    else
                    {
                        sum = Max(sum, child->fixed_size.v[axis]);
                    }
                }
            }
            
            // rjf: figure out root's size on this axis
            root->fixed_size.v[axis] = sum;
        }break;
    }
}

engine_function void
UI_LayoutEnforceConstraints_InPlaceRecursive(UI_Box *root, Axis2 axis)
{
    M_Temp scratch = GetScratch(0, 0);
    
    // NOTE(rjf): The "layout axis" is the direction in which children
    // of some node are intended to be laid out.
    
    //- rjf: fixup children sizes (if we're solving along the *non-layout* axis)
    if(axis != root->child_layout_axis && !(root->flags & (UI_BoxFlag_AllowOverflowX << axis)))
    {
        F32 allowed_size = root->fixed_size.v[axis];
        for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
        {
            if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
            {
                F32 child_size = child->fixed_size.v[axis];
                F32 violation = child_size - allowed_size;
                F32 max_fixup = child_size - child->min_size[axis];
                F32 fixup = Clamp(0, violation, max_fixup);
                if(fixup > 0)
                {
                    child->fixed_size.v[axis] -= fixup;
                }
            }
        }
        
    }
    
    //- rjf: fixup children sizes (in the direction of the layout axis)
    if(axis == root->child_layout_axis && !(root->flags & (UI_BoxFlag_AllowOverflowX << axis)))
    {
        // rjf: figure out total allowed size & total size
        F32 total_allowed_size = root->fixed_size.v[axis];
        F32 total_size = 0;
        F32 total_weighted_size = 0;
        for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
        {
            if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
            {
                total_size += child->fixed_size.v[axis];
                total_weighted_size += child->fixed_size.v[axis] * (1-child->pref_size[axis].strictness);
            }
        }
        
        // rjf: if we have a violation, we need to subtract some amount from all children
        F32 violation = total_size - total_allowed_size;
        if(violation > 0)
        {
            // rjf: figure out how much we can take in totality
            F32 child_fixup_sum = 0;
            F32 *child_fixups = PushArrayZero(scratch.arena, F32, root->child_count);
            {
                U64 child_idx = 0;
                for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next, child_idx += 1)
                {
                    if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
                    {
                        F32 fixup_size_this_child = child->fixed_size.v[axis] * (1-child->pref_size[axis].strictness);
                        if(child->fixed_size.v[axis] - fixup_size_this_child < child->min_size[axis])
                        {
                            fixup_size_this_child = child->fixed_size.v[axis] - child->min_size[axis];
                        }
                        fixup_size_this_child = ClampBot(0, fixup_size_this_child);
                        child_fixups[child_idx] = fixup_size_this_child;
                        child_fixup_sum += fixup_size_this_child;
                    }
                }
            }
            
            // rjf: fixup child sizes
            {
                U64 child_idx = 0;
                for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next, child_idx += 1)
                {
                    if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
                    {
                        F32 fixup_pct = (violation / total_weighted_size);
                        fixup_pct = Clamp(0, fixup_pct, 1);
                        child->fixed_size.v[axis] -= child_fixups[child_idx] * fixup_pct;
                    }
                }
            }
        }
        
    }
    
    //- rjf: recurse
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        UI_LayoutEnforceConstraints_InPlaceRecursive(child, axis);
    }
    
    ReleaseScratch(scratch);
}

engine_function void
UI_LayoutPosition_InPlaceRecursive(UI_Box *root, Axis2 axis)
{
    F32 layout_position = 0;
    
    //- rjf: lay out children
    F32 bounds = 0;
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        // rjf: grab original position
        F32 original_position = Min(child->rect.p0.v[axis], child->rect.p1.v[axis]);
        
        // rjf: calculate fixed position & size
        if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
        {
            child->fixed_position.v[axis] = layout_position;
            if(root->child_layout_axis == axis)
            {
                layout_position += child->fixed_size.v[axis];
                bounds += child->fixed_size.v[axis];
            }
            else
            {
                bounds = Max(bounds, child->fixed_size.v[axis]);
            }
        }
        
        // rjf: determine final rect for child, given fixed_position & size
        if(child->flags & (UI_BoxFlag_AnimatePosX<<axis))
        {
            if(child->first_touched_frame_index == child->last_touched_frame_index)
            {
                child->fixed_position_animated = child->fixed_position;
            }
            child->rect.p0.v[axis] = root->rect.p0.v[axis] + child->fixed_position_animated.v[axis] - root->view_off.v[axis];
        }
        else
        {
            child->rect.p0.v[axis] = root->rect.p0.v[axis] + child->fixed_position.v[axis] - root->view_off.v[axis];
        }
        child->rect.p1.v[axis] = child->rect.p0.v[axis] + child->fixed_size.v[axis];
        
        // rjf: grab new position
        F32 new_position = Min(child->rect.p0.v[axis], child->rect.p1.v[axis]);
        
        // rjf: store position delta
        child->position_delta.v[axis] = new_position - original_position;
    }
    
    //- rjf: store view bounds
    {
        root->view_bounds.v[axis] = bounds;
    }
    
    //- rjf: recurse
    for(UI_Box *child = root->first; !UI_BoxIsNil(child); child = child->next)
    {
        UI_LayoutPosition_InPlaceRecursive(child, axis);
    }
}

////////////////////////////////
//~ rjf: Box Building API

engine_function UI_Box *
UI_PushParent(UI_Box *box)
{
    UI_Box *last_parent = ui_state->parent_stack_active;
    ui_state->parent_stack[ui_state->parent_stack_size] = ui_state->parent_stack_active;
    ui_state->parent_stack_active = box;
    ui_state->parent_stack_size += 1;
    return last_parent;
}

engine_function UI_Box *
UI_PopParent(void)
{
    UI_Box *popped = ui_state->parent_stack_active;
    ui_state->parent_stack_size -= 1;
    ui_state->parent_stack_active = ui_state->parent_stack[ui_state->parent_stack_size];
    return popped;
}

engine_function Vec2F32 *
UI_PushViewOffStorage(Vec2F32 *v)
{
    Vec2F32 *last = ui_state->view_off_storage_stack_active;
    ui_state->view_off_storage_stack[ui_state->view_off_storage_stack_size] = ui_state->view_off_storage_stack_active;
    ui_state->view_off_storage_stack_active = v;
    ui_state->view_off_storage_stack_size += 1;
    return last;
}

engine_function Vec2F32 *
UI_PopViewOffStorage(void)
{
    Vec2F32 *popped = ui_state->view_off_storage_stack_active;
    ui_state->view_off_storage_stack_size -= 1;
    ui_state->view_off_storage_stack_active = ui_state->view_off_storage_stack[ui_state->view_off_storage_stack_size];
    return popped;
}

engine_function void
UI_BeginTooltip(void)
{
    UI_PushParent(UI_RootFromState(ui_state));
    UI_PushParent(ui_state->tooltip_root);
    UI_Box *hover_box = UI_BoxFromKey(ui_state->hot_box_key);
    UI_Box *active_box = UI_BoxFromKey(ui_state->active_box_key);
    UI_PushPrefWidth(UI_TextDim(10.f, 1.f));
    UI_PushPrefHeight(UI_TextDim(10.f, 1.f));
}

engine_function void
UI_EndTooltip(void)
{
    UI_PopPrefWidth();
    UI_PopPrefHeight();
    UI_PopParent();
    UI_PopParent();
}

engine_function UI_Box *
UI_ActiveParent(void)
{
    return ui_state->parent_stack_active;
}

engine_function UI_Box *
UI_BoxFromKey(UI_Key key)
{
    UI_Box *result = &ui_g_nil_box;
    U64 slot = key.u64[0] % ui_state->box_table_size;
    for(UI_Box *b = ui_state->box_table[slot].hash_first; !UI_BoxIsNil(b); b = b->hash_next)
    {
        if(UI_KeyMatch(b->key, key))
        {
            result = b;
            break;
        }
    }
    return result;
}

engine_function UI_Box *
UI_BoxMake(UI_BoxFlags flags, String8 string)
{
    //- rjf: grab active parent
    UI_Box *parent = ui_state->parent_stack_active;
    
    //- rjf: figure out key
    UI_Key key = UI_KeyFromString(parent->key, string);
    
    //- rjf: try to get box
    UI_Box *box = UI_BoxFromKey(key);
    B32 box_first_frame = UI_BoxIsNil(box);
    
    //- rjf: default to non-interactable box if this key is a duplicate in this frame
    if(!box_first_frame && box->last_touched_frame_index == ui_state->frame_index)
    {
        box = &ui_g_nil_box;
        box_first_frame = 1;
        key = UI_KeyNull();
    }
    
    //- rjf: gather info from box
    B32 box_is_transient = UI_KeyMatch(key, UI_KeyNull());
    Vec2F32 *last_view_off_storage = box->view_off_storage;
    
    //- rjf: allocate box if it doesn't yet exist
    if(box_first_frame)
    {
        box = ui_state->first_free_box;
        if(!UI_BoxIsNil(box))
        {
            ui_state->first_free_box = ui_state->first_free_box->next;
            MemoryZeroStruct(box);
        }
        else
        {
            box = PushArrayZero(box_is_transient ? UI_FrameArena() : ui_state->arena, UI_Box, 1);
        }
    }
    
    //- rjf: zero out per-frame state
    {
        box->first = box->last = box->next = box->prev = box->parent = &ui_g_nil_box;
        box->child_count = 0;
        box->flags = 0;
        box->view_off_storage = 0;
        MemoryZeroArray(box->pref_size);
        MemoryZeroStruct(&box->draw_bucket);
        MemoryZeroStruct(&box->draw_bucket_relative);
    }
    
    //- rjf: hook into table
    if(box_first_frame && !UI_KeyMatch(UI_KeyNull(), key))
    {
        U64 slot = key.u64[0] % ui_state->box_table_size;
        DLLInsert_NPZ(ui_state->box_table[slot].hash_first, ui_state->box_table[slot].hash_last, ui_state->box_table[slot].hash_last, box, hash_next, hash_prev, UI_BoxIsNil, UI_BoxSetNil);
    }
    
    //- rjf: hook into per-frame tree structure
    if(!UI_BoxIsNil(parent))
    {
        DLLPushBack_NPZ(parent->first, parent->last, box, next, prev, UI_BoxIsNil, UI_BoxSetNil);
        parent->child_count += 1;
        box->parent = parent;
    }
    
    //- rjf: fill from parameters
    box->key = key;
    box->flags = flags;
    box->string = PushStr8Copy(UI_FrameArena(), string);
    box->flags |= ui_state->flags.active;
    
    //- rjf: fill from context
    if(box_first_frame)
    {
        box->first_touched_frame_index = ui_state->frame_index;
    }
    box->last_touched_frame_index = ui_state->frame_index;
    
    //- rjf: fill from stacks
    {
        if(ui_state->fixed_x.count != 0)
        {
            box->flags |= UI_BoxFlag_FloatingX;
            box->fixed_position.x = ui_state->fixed_x.active;
        }
        if(ui_state->fixed_y.count != 0)
        {
            box->flags |= UI_BoxFlag_FloatingY;
            box->fixed_position.y = ui_state->fixed_y.active;
        }
        if(ui_state->fixed_width.count != 0)
        {
            box->flags |= UI_BoxFlag_FixedWidth;
            box->fixed_size.x = ui_state->fixed_width.active;
        }
        else
        {
            box->pref_size[Axis2_X] = ui_state->pref_width.active;
        }
        if(ui_state->fixed_height.count != 0)
        {
            box->flags |= UI_BoxFlag_FixedHeight;
            box->fixed_size.y = ui_state->fixed_height.active;
        }
        else
        {
            box->pref_size[Axis2_Y] = ui_state->pref_height.active;
        }
        
        box->min_size[Axis2_X] = ui_state->min_width.active;
        box->min_size[Axis2_Y] = ui_state->min_height.active;
        box->background_color = ui_state->background_color.active;
        box->text_color = ui_state->text_color.active;
        box->border_color = ui_state->border_color.active;
        box->font = ui_state->font.active;
        box->corner_radius = ui_state->corner_radius.active;
        
        if(box->flags & (UI_BoxFlag_ViewScroll|UI_BoxFlag_AllowOverflowX|UI_BoxFlag_AllowOverflowY))
        {
            box->view_off_storage = ui_state->view_off_storage_stack_active;
        }
    }
    
    //- rjf: use view_off_storage for initial value of view_off
    if((box_first_frame || box->view_off_storage != last_view_off_storage) &&
       box->view_off_storage != 0 &&
       box->flags & (UI_BoxFlag_ViewScroll|UI_BoxFlag_AllowOverflowX|UI_BoxFlag_AllowOverflowY))
    {
        box->view_off = box->target_view_off = *box->view_off_storage;
    }
    
    //- rjf: return
    return box;
}

engine_function UI_Box *
UI_BoxMakeF(UI_BoxFlags flags, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_Box *result = UI_BoxMake(flags, string);
    ReleaseScratch(scratch);
    return result;
}

engine_function void
UI_BoxEquipString(UI_Box *box, String8 string)
{
    box->string = PushStr8Copy(UI_FrameArena(), string);
}

engine_function void
UI_BoxEquipFancyStringList(UI_Box *box, FancyStringList list)
{
    box->fancy_strings = list;
}

engine_function void
UI_BoxEquipData(UI_Box *box, String8 data)
{
    Str8ListPush(UI_FrameArena(), &box->data, data);
}

engine_function void
UI_BoxEquipChildLayoutAxis(UI_Box *box, Axis2 axis)
{
    box->child_layout_axis = axis;
}

engine_function void
UI_BoxEquipHoverCursor(UI_Box *box, OS_CursorKind cursor)
{
    box->hover_cursor = cursor;
}

engine_function void
UI_BoxEquipTexture(UI_Box *box, R_Handle texture, Rng2F32 src)
{
    box->texture = texture;
    box->texture_src = src;
}

engine_function void
UI_BoxEquipDrawBucket(UI_Box *box, DR_Bucket *bucket)
{
    box->flags |= UI_BoxFlag_DrawBucket;
    DR_CmdList(&box->draw_bucket, bucket->cmds);
}

engine_function void
UI_BoxEquipDrawBucketRelative(UI_Box *box, DR_Bucket *bucket)
{
    box->flags |= UI_BoxFlag_DrawBucketRelative;
    DR_CmdList(&box->draw_bucket_relative, bucket->cmds);
}

engine_function Vec2F32
UI_BoxTextPosition(UI_Box *box)
{
    R_Font font = box->font;
    Vec2F32 text_position =
    {
        box->rect.p0.x + 4.f,
        (box->rect.p0.y + box->rect.p1.y)/2.f - font.line_advance/2 - 1.f,
    };
    if(box->flags & UI_BoxFlag_TextAlignCenter)
    {
        String8 str = UI_DisplayStringFromBox(box);
        Vec2F32 advance = R_AdvanceFromString(font, str);
        text_position.x = (box->rect.p0.x + box->rect.p1.x)/2 - advance.x/2;
    }
    return text_position;
}

engine_function U64
UI_BoxCharPosFromXY(UI_Box *box, Vec2F32 xy)
{
    R_Font font = box->font;
    
    U64 result = 0;
    String8 line = UI_DisplayStringFromBox(box);
    U64 best_offset = 0;
    F32 best_distance = -1.f;
    F32 x = UI_BoxTextPosition(box).x;
    for(U64 line_char_idx = 0; line_char_idx <= line.size; line_char_idx += 1)
    {
        F32 this_char_distance = fabsf(xy.x - x);
        if(this_char_distance < best_distance || best_distance < 0.f)
        {
            best_offset = line_char_idx;
            best_distance = this_char_distance;
        }
        if(line_char_idx < line.size)
        {
            x += R_AdvanceFromString(font, Substr8(line, R1U64(line_char_idx, line_char_idx+1))).x;
        }
    }
    result = best_offset;
    return result;
}


////////////////////////////////
//~ rjf: Stacks

#define UI_StackPush(name, new_top)\
(\
(ui_state->name.v[ui_state->name.count] = ui_state->name.active),\
(ui_state->name.active = new_top),\
(ui_state->name.count += 1),\
(ui_state->name.v[ui_state->name.count-1])\
)
#define UI_StackPop(name)\
(\
(ui_state->name.count -= 1),\
(ui_state->name.active = ui_state->name.v[ui_state->name.count]),\
(ui_state->name.v[ui_state->name.count+1])\
)

//- rjf: base 

engine_function F32 UI_PushFixedX(F32 v)                    { return UI_StackPush(fixed_x, v);          }
engine_function F32 UI_PopFixedX(void)                      { return UI_StackPop(fixed_x);              }
engine_function F32 UI_PushFixedY(F32 v)                    { return UI_StackPush(fixed_y, v);          }
engine_function F32 UI_PopFixedY(void)                      { return UI_StackPop(fixed_y);              }
engine_function F32 UI_PushFixedWidth(F32 v)                { return UI_StackPush(fixed_width, v);      }
engine_function F32 UI_PopFixedWidth(void)                  { return UI_StackPop(fixed_width);          }
engine_function F32 UI_PushFixedHeight(F32 v)               { return UI_StackPush(fixed_height, v);     }
engine_function F32 UI_PopFixedHeight(void)                 { return UI_StackPop(fixed_height);         }
engine_function UI_Size UI_PushPrefWidth(UI_Size v)         { return UI_StackPush(pref_width, v);       }
engine_function UI_Size UI_PopPrefWidth(void)               { return UI_StackPop(pref_width);           }
engine_function UI_Size UI_PushPrefHeight(UI_Size v)        { return UI_StackPush(pref_height, v);      }
engine_function UI_Size UI_PopPrefHeight(void)              { return UI_StackPop(pref_height);          }
engine_function F32 UI_PushMinWidth(F32 v)                  { return UI_StackPush(min_width, v);        }
engine_function F32 UI_PopMinWidth(void)                    { return UI_StackPop(min_width);            }
engine_function F32 UI_PushMinHeight(F32 v)                 { return UI_StackPush(min_height, v);       }
engine_function F32 UI_PopMinHeight(void)                   { return UI_StackPop(min_height);           }
engine_function UI_BoxFlags UI_PushFlags(UI_BoxFlags v)     { return UI_StackPush(flags, v);            }
engine_function UI_BoxFlags UI_PopFlags(void)               { return UI_StackPop(flags);                }
engine_function Vec4F32 UI_PushBackgroundColor(Vec4F32 v)   { return UI_StackPush(background_color, v); }
engine_function Vec4F32 UI_PopBackgroundColor(void)         { return UI_StackPop(background_color);     }
engine_function Vec4F32 UI_PushTextColor(Vec4F32 v)         { return UI_StackPush(text_color, v);       }
engine_function Vec4F32 UI_PopTextColor(void)               { return UI_StackPop(text_color);           }
engine_function Vec4F32 UI_PushBorderColor(Vec4F32 v)       { return UI_StackPush(border_color, v);     }
engine_function Vec4F32 UI_PopBorderColor(void)             { return UI_StackPop(border_color);         }
engine_function R_Font UI_PushFont(R_Font v)                { return UI_StackPush(font, v);             }
engine_function R_Font UI_PopFont(void)                     { return UI_StackPop(font);                 }
engine_function F32 UI_PushCornerRadius(F32 v)              { return UI_StackPush(corner_radius, v);    }
engine_function F32 UI_PopCornerRadius(void)                { return UI_StackPop(corner_radius);        }

//- rjf: helpers

engine_function Rng2F32
UI_PushRect(Rng2F32 rect)
{
    Rng2F32 replaced = {0};
    Vec2F32 size = Dim2F32(rect);
    replaced.x0 = UI_PushFixedX(rect.x0);
    replaced.y0 = UI_PushFixedY(rect.y0);
    replaced.x1 = replaced.x0 + UI_PushFixedWidth(size.x);
    replaced.y1 = replaced.y0 + UI_PushFixedHeight(size.y);
    return replaced;
}

engine_function Rng2F32
UI_PopRect(void)
{
    Rng2F32 popped = {0};
    popped.x0 = UI_PopFixedX();
    popped.y0 = UI_PopFixedY();
    popped.x1 = popped.x0 + UI_PopFixedWidth();
    popped.y1 = popped.y0 + UI_PopFixedHeight();
    return popped;
}

engine_function UI_Size
UI_PushPrefSize(Axis2 axis, UI_Size v)
{
    return (axis == Axis2_X ? UI_PushPrefWidth : UI_PushPrefHeight)(v);
}

engine_function UI_Size
UI_PopPrefSize(Axis2 axis)
{
    return (axis == Axis2_X ? UI_PopPrefWidth : UI_PopPrefHeight)();
}

////////////////////////////////
//~ rjf: Box Interaction

engine_function UI_InteractResult
UI_BoxInteract(UI_Box *box)
{
    UI_InteractResult result = {0};
    result.box = box;
    Vec2F32 mouse = OS_MouseFromWindow(ui_state->window);
    B32 mouse_is_over = Contains2F32(box->rect, mouse);
    result.mouse = mouse;
    
    //- rjf: check for parent that is clipping
    if(box->flags & (UI_BoxFlag_Clickable|UI_BoxFlag_ViewScroll) && mouse_is_over)
    {
        for(UI_Box *parent = box->parent; !UI_BoxIsNil(parent); parent = parent->parent)
        {
            if(parent->flags & UI_BoxFlag_Clip)
            {
                mouse_is_over = mouse_is_over && Contains2F32(parent->rect, mouse);
                break;
            }
        }
    }
    
    //- rjf: clickability
    if(box->flags & UI_BoxFlag_Clickable)
    {
        
        // rjf: hot management
        if(UI_KeyMatch(UI_KeyNull(), ui_state->active_box_key) &&
           UI_KeyMatch(UI_KeyNull(), ui_state->hot_box_key) &&
           mouse_is_over)
        {
            ui_state->hot_box_key = box->key;
        }
        else if(UI_KeyMatch(ui_state->hot_box_key, box->key) &&
                !mouse_is_over)
        {
            ui_state->hot_box_key = UI_KeyNull();
        }
        
        // rjf: active management
        if(UI_KeyMatch(ui_state->hot_box_key, box->key) &&
           UI_KeyMatch(ui_state->active_box_key, UI_KeyNull()) &&
           OS_KeyPress(ui_state->events, ui_state->window, OS_Key_MouseLeft, 0))
        {
            result.pressed = 1;
            ui_state->active_box_key = box->key;
            ui_g_drag_active = 1;
        }
        else if(UI_KeyMatch(ui_state->active_box_key, box->key) &&
                OS_KeyRelease(ui_state->events, ui_state->window, OS_Key_MouseLeft, 0))
        {
            result.released = 1;
            result.clicked = mouse_is_over;
            ui_state->active_box_key = UI_KeyNull();
        }
        
        // rjf: dragging
        if(UI_KeyMatch(ui_state->active_box_key, box->key))
        {
            result.dragging = 1;
            if(result.pressed)
            {
                ui_state->drag_start_mouse = mouse;
            }
            result.drag_delta = Sub2F32(mouse, ui_state->drag_start_mouse);
        }
        
    }
    
    //- rjf: scrolling
    if(box->flags & UI_BoxFlag_ViewScroll && mouse_is_over)
    {
        OS_EventList *events = UI_Events();
        for(OS_Event *event = events->first, *next = 0; event != 0; event = next)
        {
            next = event->next;
            if(OS_HandleMatch(event->window, ui_state->window) && event->kind == OS_EventKind_MouseScroll)
            {
                OS_EatEvent(events, event);
                box->target_view_off.x += event->scroll.x;
                box->target_view_off.y += event->scroll.y;
            }
        }
    }
    
    //- rjf: set hovering status
    result.hovering = mouse_is_over;
    
    return result;
}
