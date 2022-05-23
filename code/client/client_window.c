////////////////////////////////
//~ rjf: Generated Code

#include "client/generated/client_window.meta.c"

////////////////////////////////
//~ rjf: Globals

global M_Arena *cw_g_keybind_arena = 0;
global CFG_CachedFile *cw_g_keybind_file = 0;
global U64 cw_g_keybind_generation = 0;
global CW_KeybindTable cw_g_keybind_table = {0};

////////////////////////////////
//~ rjf: Commands

function CW_CmdKind
CW_CmdKindFromString(String8 string)
{
    CW_CmdKind result = CW_CmdKind_Null;
    for(U64 idx = 0; idx < ArrayCount(cw_g_cmd_kind_string_table); idx += 1)
    {
        if(Str8Match(string, cw_g_cmd_kind_string_table[idx], MatchFlag_CaseInsensitive))
        {
            result = (CW_CmdKind)idx;
            break;
        }
    }
    return result;
}

function void
CW_DoCmdFastPath(CW_State *state, C_Panel *panel, CW_CmdKind kind)
{
    M_Temp scratch = GetScratch(0, 0);
    CW_CmdFastPath fastpath = cw_g_cmd_kind_fast_path_table[kind];
    String8 insert_suffix = {0};
    state->cmd_reg.panel = panel;
    switch(fastpath)
    {
        case CW_CmdFastPath_Run:
        {
            CW_PushCmd(state, cw_g_cmd_kind_string_table[kind]);
        }break;
        
        case CW_CmdFastPath_PutName:{}goto put_name;
        case CW_CmdFastPath_PutNameSpace:
        {
            insert_suffix = Str8Lit(" ");
            panel->input_is_focused = 1;
        }goto put_name;
        case CW_CmdFastPath_PutNamePath:
        {
            String8 path = OS_GetSystemPath(scratch.arena, OS_SystemPath_Current);
            for(U64 idx = 0; idx < path.size; idx += 1)
            {
                path.str[idx] = CharToForwardSlash(path.str[idx]);
            }
            insert_suffix = PushStr8F(scratch.arena, " %.*s/", Str8VArg(path));
            panel->input_is_focused = 1;
        }goto put_name;
        case CW_CmdFastPath_PutNameViewCommand:
        {
            // insert_suffix = PushStr8F(scratch.arena, " %.*s", Str8VArg(panel->selected_view->command));
            panel->input_is_focused = 1;
        }goto put_name;
        put_name:;
        {
            String8 insert_string = PushStr8F(scratch.arena, "%.*s%.*s", Str8VArg(cw_g_cmd_kind_string_table[kind]), Str8VArg(insert_suffix));
            C_PanelFillInput(panel, insert_string);
            panel->input_is_focused = 1;
        }break;
    }
    ReleaseScratch(scratch);
}

function void
CW_PushCmd(CW_State *state, String8 string)
{
    C_CmdNode *node = PushArrayZero(state->cmd_arena, C_CmdNode, 1);
    node->cmd.reg = state->cmd_reg;
    node->cmd.string = PushStr8Copy(state->cmd_arena, string);
    DLLPushBack(state->cmd_batch.first, state->cmd_batch.last, node);
    state->cmd_batch.count += 1;
}

function void
CW_PushCmdF(CW_State *state, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    CW_PushCmd(state, string);
    ReleaseScratch(scratch);
}

function C_View *
CW_ViewFromPanel(C_Panel *panel)
{
    C_View *result = panel->selected_view;
    if(C_ViewIsNil(result) || panel->input.size != 0)
    {
        result = panel->transient_view;
    }
    if(result == panel->transient_view && !C_ViewIsNil(panel->selected_view))
    {
        M_Temp scratch = GetScratch(0, 0);
        CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, result->command);
        String8 cmd_string = cmd_node->string;
        CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
        CW_ViewKind view_kind = cw_g_cmd_kind_view_kind_table[cmd_kind];
        if(view_kind == CW_ViewKind_Null && cmd_kind != CW_CmdKind_Null)
        {
            result = panel->selected_view;
        }
        ReleaseScratch(scratch);
    }
    return result;
}

////////////////////////////////
//~ rjf: Keybindings

function U64
CW_HashFromKeyAndModifiers(OS_Key key, OS_Modifiers modifiers)
{
    U32 lower_32 = key;
    U32 upper_32 = modifiers;
    U64 result = (U64)lower_32 | ((U64)upper_32 << 32);
    return result;
}

function CW_Keybind
CW_KeybindMake(CW_CmdKind cmd, OS_Key key, OS_Modifiers modifiers)
{
    CW_Keybind keybind = {0};
    keybind.cmd = cmd;
    keybind.key = key;
    keybind.modifiers = modifiers;
    return keybind;
}

function CW_KeybindTable
CW_KeybindTableMake(M_Arena *arena, U64 table_size)
{
    CW_KeybindTable table = {0};
    table.table_size = table_size;
    table.table = PushArrayZero(arena, CW_KeybindNode *, table.table_size);
    return table;
}

function void
CW_KeybindTableInsert(M_Arena *arena, CW_KeybindTable *table, CW_Keybind keybind)
{
    U64 hash = CW_HashFromKeyAndModifiers(keybind.key, keybind.modifiers);
    U64 slot = hash % table->table_size;
    CW_KeybindNode *n = PushArrayZero(arena, CW_KeybindNode, 1);
    n->hash_next = table->table[slot];
    n->keybind = keybind;
    table->table[slot] = n;
}

function CW_CmdKind
CW_KeybindTableLookup(CW_KeybindTable *table, OS_Key key, OS_Modifiers modifiers)
{
    CW_CmdKind cmd_kind = CW_CmdKind_Null;
    if(table->table_size != 0)
    {
        U64 hash = CW_HashFromKeyAndModifiers(key, modifiers);
        U64 slot = hash % table->table_size;
        for(CW_KeybindNode *n = table->table[slot]; n != 0; n = n->hash_next)
        {
            if(n->keybind.key == key && n->keybind.modifiers == modifiers)
            {
                cmd_kind = n->keybind.cmd;
                break;
            }
        }
    }
    return cmd_kind;
}

////////////////////////////////
//~ rjf: Custom UI Widgets

function UI_Box *
CW_TabBegin(String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|
                             UI_BoxFlag_DrawBackground|
                             UI_BoxFlag_DrawBorder|
                             UI_BoxFlag_DrawText|
                             UI_BoxFlag_DrawHotEffects|
                             UI_BoxFlag_AnimatePosX,
                             string);
    UI_BoxEquipHoverCursor(box, OS_CursorKind_Hand);
    UI_PushParent(box);
    return box;
}

function UI_Box *
CW_TabBeginF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_Box *result = CW_TabBegin(string);
    ReleaseScratch(scratch);
    return result;
}

function UI_InteractResult
CW_TabEnd(void)
{
    UI_Box *box = UI_PopParent();
    UI_InteractResult interact = UI_BoxInteract(box);
    return interact;
}

function UI_InteractResult
CW_LineEdit(B32 keyboard_focused, TE_Point *cursor, TE_Point *mark, U64 string_max, String8 *out_string, String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|
                             UI_BoxFlag_DrawBackground|
                             UI_BoxFlag_DrawBorder|
                             UI_BoxFlag_DrawText,
                             string);
    UI_BoxEquipHoverCursor(box, OS_CursorKind_IBar);
    
    //- rjf: use edited string if it's not empty
    if(out_string->size != 0)
    {
        UI_BoxEquipString(box, *out_string);
        box->flags |= UI_BoxFlag_UseFullString;
    }
    
    //- rjf: interact
    UI_InteractResult interact = UI_BoxInteract(box);
    if(interact.dragging)
    {
        TE_Point mouse_text_point = TE_PointMake(1, UI_BoxCharPosFromXY(box, interact.mouse)+1);
        if(interact.pressed)
        {
            *mark = mouse_text_point;
            UI_ResetCaretBlinkT();
        }
        *cursor = mouse_text_point;
    }
    
    //- rjf: extra cursor/mark rendering
    if(keyboard_focused)
    {
        R_Font font = box->font;
        F32 blink_f = cosf(0.5f * 3.14159f * UI_CaretBlinkT() / OS_CaretBlinkTime());
        blink_f = blink_f * blink_f;
        Vec4F32 cursor_color = V4F32(1, 1, 1, 0.8*(F32)keyboard_focused*blink_f);
        Vec4F32 selection_color = V4F32(0.4f, 0.7f, 1, 0.2*(F32)keyboard_focused);
        Vec2F32 text_position = UI_BoxTextPosition(box);
        F32 cursor_pixel_off = R_AdvanceFromString(font, Prefix8(*out_string, cursor->column-1)).x;
        F32 mark_pixel_off = R_AdvanceFromString(font, Prefix8(*out_string, mark->column-1)).x;
        Rng2F32 cursor_rect =
        {
            text_position.x-1.f + cursor_pixel_off,
            box->rect.y0+2.f,
            text_position.x+1.f + cursor_pixel_off,
            box->rect.y1-2.f,
        };
        Rng2F32 mark_rect =
        {
            text_position.x-1.f + mark_pixel_off,
            box->rect.y0+2.f,
            text_position.x+1.f + mark_pixel_off,
            box->rect.y1-2.f,
        };
        Rng2F32 selection_rect = Union2F32(cursor_rect, mark_rect);
        
        DR_Bucket bucket = {0};
        DR_Rect_C(&bucket, cursor_rect, cursor_color, 0);
        DR_Rect_C(&bucket, selection_rect, selection_color, 4.f);
        UI_BoxEquipDrawBucket(box, &bucket);
    }
    
    //- rjf: equip string again, to make sure we render the right thing post-input
    if(out_string->size != 0)
    {
        box->flags |= UI_BoxFlag_UseFullString;
        UI_BoxEquipString(box, *out_string);
    }
    
    return interact;
}

function UI_InteractResult
CW_LineEditF(B32 keyboard_focused, TE_Point *cursor, TE_Point *mark, U64 string_max, String8 *out_string, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = CW_LineEdit(keyboard_focused, cursor, mark, string_max, out_string, string);
    ReleaseScratch(scratch);
    return itrct;
}

////////////////////////////////
//~ rjf: Drag/Drop

global C_Handle cw_g_view_drag_src_panel_h = {0};
global C_Handle cw_g_view_drag_view_h = {0};

function B32
CW_ViewDragIsActive(void)
{
    C_View *view = C_ViewFromHandle(cw_g_view_drag_view_h);
    return !C_ViewIsNil(view);
}

function C_View *
CW_DraggedView(void)
{
    C_View *view = C_ViewFromHandle(cw_g_view_drag_view_h);
    return view;
}

function void
CW_ViewDragBegin(C_Panel *panel, C_View *view)
{
    cw_g_view_drag_src_panel_h = C_HandleFromPanel(panel);
    cw_g_view_drag_view_h = C_HandleFromView(view);
}

function void
CW_ViewDragEnd(C_Panel *drop_panel, C_View *prev_view)
{
    if(!C_PanelIsNil(drop_panel))
    {
        C_View *view = C_ViewFromHandle(cw_g_view_drag_view_h);
        if(prev_view == view)
        {
            prev_view = prev_view->prev;
        }
        C_Panel *src_panel = C_PanelFromHandle(cw_g_view_drag_src_panel_h);
        C_PanelRemoveView(src_panel, view);
        C_PanelInsertView(drop_panel, prev_view, view);
        drop_panel->selected_view = view;
    }
    cw_g_view_drag_src_panel_h = C_HandleFromPanel(&c_g_nil_panel);
    cw_g_view_drag_view_h = C_HandleFromView(&c_g_nil_view);
}

////////////////////////////////
//~ rjf: View Hook Implementations

CW_VIEW_FUNCTION_DEF(Null)
{
}

CW_VIEW_FUNCTION_DEF(FileSystem)
{
    M_Temp scratch = GetScratch(0, 0);
    String8 cmd_past_name = Str8SkipChopWhitespace(Str8Skip(cmd_str, cmd_node->range_in_input.max));
    String8 search_path_name = Str8ChopLastSlash(cmd_past_name);
    String8 search_path = PushStr8F(scratch.arena, "%S/", search_path_name);
    String8 search_query = Str8SkipLastSlash(cmd_past_name);
    Vec4F32 weak_text_color = TM_ThemeColorFromString(&state->theme, Str8Lit("base_text_weak"));
    Vec4F32 error_text_color = TM_ThemeColorFromString(&state->theme, Str8Lit("error_text"));
    
    // rjf: make file buttons
    OS_FileIter *it = OS_FileIterBegin(scratch.arena, search_path);
    U64 num_found = 0;
    for(OS_FileInfo info = {0}; OS_FileIterNext(scratch.arena, it, &info);)
    {
        if(!Str8Match(info.name, Str8Lit("."), 0) &&
           !Str8Match(info.name, Str8Lit(".."), 0) &&
           (search_query.size == 0 || FindSubstr8(info.name, search_query, 0, MatchFlag_CaseInsensitive) < info.name.size))
        {
            num_found += 1;
            UI_InteractResult interact = UI_ButtonF("###%S", info.name);
            UI_PushParent(interact.box);
            {
                B32 is_directory = info.attributes.flags & OS_FileFlag_Directory;
                B32 hovered = UI_KeyMatch(interact.box->key, UI_HotKey());
                UI_Font(APP_IconsFont()) UI_TextColor(weak_text_color)
                    UI_PrefWidth(UI_Pixels(20.f, 1.f)) UI_LabelF(is_directory && hovered ?  "N" :
                                                                 is_directory && !hovered ? "M" :
                                                                 "f");
                UI_LabelF("%S", info.name);
            }
            UI_PopParent();
        }
    }
    if(num_found == 0)
    {
        UI_TextColor(error_text_color)
            UI_LabelF("No files found.");
    }
    OS_FileIterEnd(it);
    ReleaseScratch(scratch);
}

#if 0
CW_VIEW_FUNCTION_DEF(Code)
{
    M_Temp scratch = GetScratch(0, 0);
    C_Entity *entity = C_EntityFromCommandString(view->command);
    C_LangRuleSet *lang_rules = C_LangRuleSetFromEntity(entity);
    C_Buffer *buffer = entity->buffer;
    R_Font font = APP_DefaultFont();
    
    CMD_Node *query_cmd_node = CMD_ParseNodeFromString(scratch.arena, panel->input);
    String8 cmd_string = query_cmd_node->string;
    CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
    
    C_TextLayoutCache *tl_cache = view->text_layout_cache;
    
    B32 do_line_wrapping = CFG_B32FromKey(Str8Lit("line_wrapping"));
    
    // rjf: experimental literal editor UI
#if 1
    switch(cmd_kind)
    {
        default:break;
        case CW_CmdKind_ColorPick:
        {
            TE_Point cursor = C_CursorFromCursorState(C_MainCursorStateFromView(view));
            
            // rjf: first, try the enclosing range
            C_TokenRangeNodeLoose *rng_root = C_TokenRangeTreeFromBuffer_Loose(buffer);
            C_TokenRangeNodeLoose *cursor_rng = C_EnclosingTokenRangeNodeFromPoint_Loose(rng_root, cursor);
            if(rng_root != 0 && cursor_rng != rng_root && cursor_rng->range.range.max.line == cursor_rng->range.range.min.line)
            {
                String8 line_str = C_StringFromBufferLineNum(scratch.arena, buffer, cursor.line);
                String8 range_substr = Substr8(line_str, R1U64(cursor_rng->range.range.min.column-1, cursor_rng->range.range.max.column-1));
                MD_ParseResult parse = MD_ParseOneNode(scratch.arena, MDFromStr8(range_substr), 0);
                MD_Node *node = parse.node;
                if(MD_ChildCountFromNode(node) >= 3 &&
                   node->first_child->flags & MD_NodeFlag_Numeric &&
                   node->first_child->next->flags & MD_NodeFlag_Numeric &&
                   node->first_child->next->next->flags & MD_NodeFlag_Numeric)
                {
                    Vec3F32 rgb =
                    {
                        (F32)F64FromStr8(Str8FromMD(node->first_child->string)),
                        (F32)F64FromStr8(Str8FromMD(node->first_child->next->string)),
                        (F32)F64FromStr8(Str8FromMD(node->first_child->next->next->string)),
                    };
                    Vec3F32 hsv = HSVFromRGB(rgb);
                    Vec2F32 line_advance = R_AdvanceFromString(font, line_str);
                    UI_WindowBegin(R2F32(V2F32(line_advance.x,       cursor.line*font.line_advance),
                                         V2F32(line_advance.x + 200, cursor.line*font.line_advance + 200)),
                                   Str8Lit("color_picker"));
                    UI_HeightFill
                        UI_SaturationValuePicker(hsv.x, &hsv.y, &hsv.z, Str8Lit("satval"));
                    UI_HuePicker(&hsv.x, Str8Lit("hue"));
                    UI_WindowEnd();
                    
                    Vec3F32 new_rgb = RGBFromHSV(hsv);
                    String8 replaced_stuff = PushStr8F(scratch.arena, "(%.2ff, %.2ff, %.2ff, 1.00f)", new_rgb.x, new_rgb.y, new_rgb.z);
                    C_EntityReplaceRange(lang_rules, entity, cursor_rng->range.range, replaced_stuff);
                }
            }
            
            // rjf: try individual numeric literal
#if 0
            C_TokenList tokens = C_TokenListFromEntityLineNum(scratch.arena, cursor.line);
            C_Token cursor_token = C_TokenFromPointInList(cursor.column-1, tokens);
            if(cursor_token.kind == C_TokenKind_NumericLiteral)
            {
                
            }
#endif
        }break;
    }
#endif
    
    // rjf: get find string
    String8 find_string = {0};
    {
        switch(cmd_kind)
        {
            default:break;
            case CW_CmdKind_FindNext:
            case CW_CmdKind_FindPrev:
            case CW_CmdKind_FindAll:
            {
                find_string = Str8SkipWhitespace(Str8Skip(panel->input, query_cmd_node->range_in_input.max));
            }break;
        }
    }
    
    // rjf: apply text operations
    if(keyboard_focused)
    {
        for(TE_InputActionNode *n = text_input_actions.first; n != 0; n = n->next)
        {
            TE_InputAction action = n->action;
            
            // rjf: figure out if this action is nontrivial
            B32 action_is_nontrivial = !!(action.flags & (TE_InputActionFlag_Delete|TE_InputActionFlag_Paste) || action.codepoint != 0);
            
            // rjf: calculate glue code
            U64 glue_code = 0;
            {
                if(CharIsAlpha(action.codepoint) || CharIsDigit(action.codepoint))
                {
                    glue_code = 1;
                }
                else if(CharIsSpace(action.codepoint))
                {
                    glue_code = 2;
                }
                else if(CharIsSymbol(action.codepoint))
                {
                    glue_code = 3;
                }
            }
            
            // rjf: push change record on non-trivial edits
            M_Arena *change_arena = C_ChangeStackArenaFromBufferDirection(buffer, C_ChangeDirection_Undo);
            C_ChangeRecord *change_record = 0;
            if(action_is_nontrivial)
            {
                C_ChangeRecord *top_record = C_ChangeStackTopFromBufferDirection(buffer, C_ChangeDirection_Undo);
                if(top_record != 0 && top_record->glue_code == glue_code && glue_code != 0)
                {
                    change_record = top_record;
                }
                else
                {
                    change_record = C_PushChangeRecord(buffer, C_ChangeDirection_Undo, glue_code);
                }
            }
            
            // rjf: apply ops for each cursor
            for(C_CursorState *cs = view->cursor_states.first; cs != 0; cs = cs->next)
            {
                TE_Point cursor = C_CursorFromCursorState(cs);
                TE_Point mark = C_MarkFromCursorState(cs);
                
                // rjf: transform action & concrete data into operation we need to apply
                TE_Op op = {0};
                {
                    // rjf: first try to map to a single-line op
                    {
                        op = CW_SingleLineOpFromInputAction(scratch.arena, action, cursor, mark, cs->preferred_column, buffer, tl_cache);
                    }
                    
                    // rjf: if single-line failed, map to a multi-line op
                    if(op.taken == 0)
                    {
                        op = CW_MultiLineOpFromInputAction(scratch.arena, action, cursor, mark, cs->preferred_column, buffer, tl_cache);
                    }
                    else
                    {
                        op.new_preferred_column = op.cursor.column;
                    }
                }
                
                // rjf: determine replace + whether or not to use the inserted range to position
                // our cursor/mark
                String8 replace = op.replace;
                B32 use_inserted_range = 0;
                {
                    if(op.paste)
                    {
                        replace = OS_GetClipboardText(scratch.arena);
                        use_inserted_range = 1;
                    }
                    if(op.copy && cs == view->cursor_states.first)
                    {
                        TE_Point cursor = C_CursorFromCursorState(cs);
                        TE_Point mark = C_MarkFromCursorState(cs);
                        String8List bake_list = C_BakedStringListFromBufferRange(scratch.arena, buffer, TE_RangeMake(cursor, mark));
                        String8 bake = Str8ListJoin(scratch.arena, bake_list, 0);
                        OS_SetClipboardText(bake);
                    }
                }
                
                // rjf: get text that we're replacing
                String8 replaced_text = {0};
                if(action_is_nontrivial)
                {
                    String8List strs = C_BakedStringListFromBufferRange(scratch.arena, buffer, op.range);
                    replaced_text = Str8ListJoin(scratch.arena, strs, 0);
                }
                
                // rjf: apply op
                TE_Range inserted_range = {0};
                if(op.taken != 0)
                {
                    inserted_range = C_EntityReplaceRange(lang_rules, entity, op.range, replace);
                }
                
                // rjf: push undo ops on non-trivial edits
                if(change_record != 0)
                {
                    C_PushTextOp(change_arena, change_record, inserted_range, replaced_text);
                    C_PushCursorStateOp(change_arena, change_record, cursor, mark);
                }
                
                // rjf: clear redos
                C_ClearChangeRecordStack(buffer, C_ChangeDirection_Redo);
                
                // rjf: use the inserted range to position the cursor/mark
                if(use_inserted_range)
                {
                    C_CursorStateSetCursor(cs, inserted_range.max);
                    C_CursorStateSetMark(cs, inserted_range.max);
                }
                
                // rjf: use whatever the operation produced
                else
                {
                    C_CursorStateSetCursor(cs, op.cursor);
                    C_CursorStateSetMark(cs, op.mark);
                }
                
                // rjf: apply preferred column
                cs->preferred_column = op.new_preferred_column;
                
                // rjf: don't continue to extra cursors if we don't have the mode on
                if(!(state->mode_flags & CW_ModeFlag_MultiCursor))
                {
                    break;
                }
            }
            
            UI_ResetCaretBlinkT();
            CW_PushCmd(state, cw_g_cmd_kind_string_table[CW_CmdKind_SnapToCursor]);
        }
    }
    
    // rjf: determine visible pixel range
    Rng1F32 visible_pixel_range = R1F32(parent->view_off.y, parent->view_off.y + Dim2F32(parent->rect).y);
    
    // rjf: calculate space needed for margin
    Rng1S64 last_visible_line_range =
    {
        C_LineNumFromPixelOffInCache_GroundTruth(visible_pixel_range.min, tl_cache),
        C_LineNumFromPixelOffInCache_GroundTruth(visible_pixel_range.max, tl_cache)+2,
    };
    R_Glyph large_digit_glyph = R_GlyphFromFontCodepoint(font, '8');
    S64 digits_needed_for_line_num = log10(last_visible_line_range.max)+1;
    digits_needed_for_line_num = Max(digits_needed_for_line_num, 2);
    F32 space_for_margin = large_digit_glyph.advance * (digits_needed_for_line_num+1);
    
    // rjf: update textual layout cache
    F32 width_constraint = Dim2F32(parent->rect).x - space_for_margin;
    if(do_line_wrapping == 0)
    {
        width_constraint = F32Max;
    }
    C_TextLayoutCacheUpdate(tl_cache, buffer, font, visible_pixel_range, width_constraint);
    
    // rjf: take visible pixel range => visible line range
    Rng1S64 visible_line_range =
    {
        C_LineNumFromPixelOffInCache_GroundTruth(visible_pixel_range.min, tl_cache),
        C_LineNumFromPixelOffInCache_GroundTruth(visible_pixel_range.max, tl_cache)+2,
    };
    visible_line_range.min = Clamp(1, visible_line_range.min, tl_cache->line_count);
    visible_line_range.max = Clamp(1, visible_line_range.max, tl_cache->line_count);
    
    // rjf: do space before visible lines
    {
        F32 space_before_lines = C_PixelRangeFromLineNumInCache_GroundTruth(visible_line_range.min, tl_cache).min;
        UI_Spacer(UI_Pixels(space_before_lines, 1));
    }
    
    // rjf: do code lines
    UI_InteractResult interact = CW_CodeEditF(keyboard_focused, state, view, entity, visible_line_range, space_for_margin, &state->theme, find_string, "###code_edit_%p", panel);
    
    // rjf: change focused panel on click
    if(interact.pressed)
    {
        state->next_focused_panel = panel;
    }
    
    // rjf: extra room for bottom of buffer
    {
        UI_Spacer(UI_Pixels(Dim2F32(parent->rect).y - 2*font.line_advance, 1));
    }
    
    // rjf: do space after visible lines
    {
        F32 space_after_lines = C_PixelRangeFromLineNumInCache_GroundTruth(visible_line_range.max, tl_cache).max;
        UI_Spacer(UI_Pixels(C_TotalHeightFromTextLayoutCache(tl_cache)-space_after_lines, 1));
    }
    
    ReleaseScratch(scratch);
}
#endif

////////////////////////////////
//~ rjf: Hooks

function APP_WindowUserHooks
CW_WindowUserHooks(void)
{
    APP_WindowUserHooks hooks = {0};
    hooks.Open = CW_Open;
    hooks.Close = CW_Close;
    hooks.Update = CW_Update;
    return hooks;
}

function void *
CW_Open(APP_Window *window)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(8));
    CW_State *state = PushArrayZero(arena, CW_State, 1);
    state->arena = arena;
    state->ui_state = UI_StateAlloc();
    state->panel_root = C_PanelAlloc();
    state->focused_panel = state->panel_root;
    
    // rjf: init command batch state
    {
        state->cmd_arena = M_ArenaAlloc(Gigabytes(1));
    }
    
    // rjf: init theme
    {
        state->theme = state->target_theme = TM_ThemeMake(arena);
    }
    
    return state;
}

function void
CW_Close(APP_Window *window, void *data)
{
    // TODO(rjf): we need to release all panels here.
    CW_State *state = data;
    UI_StateRelease(state->ui_state);
    M_ArenaRelease(state->arena);
}

function void
CW_Update(APP_Window *window, OS_EventList *events, void *data)
{
    CW_State *state = data;
    
    //- rjf: prep view command data
    struct {C_Panel *panel; S64 line_num;} goto_line = {0};
    struct {C_Panel *panel; F32 padding;} snap_to_cursor = {0};
    struct {TE_InputActionList actions;} text_actions = {0};
    U32 text_write_codepoint = 0;
    
    //- rjf: begin frame
    {
        if(!C_PanelIsNil(state->next_focused_panel))
        {
            state->focused_panel = state->next_focused_panel;
            state->next_focused_panel = &c_g_nil_panel;
        }
    }
    
    //- rjf: update bindings from file
    // TODO(rjf): error reporting
    {
        CFG_NodeFilePair keybindings_nfp = CFG_NodeFilePairFromKey(Str8Lit("keybindings"), 1);
        MD_Node *root = keybindings_nfp.node;
        CFG_CachedFile *keybind_file = keybindings_nfp.file;
        if(keybind_file != 0 &&
           (keybind_file != cw_g_keybind_file ||
            keybind_file->generation != cw_g_keybind_generation))
        {
            if(cw_g_keybind_arena == 0)
            {
                cw_g_keybind_arena = M_ArenaAlloc(Megabytes(64));
            }
            M_ArenaClear(cw_g_keybind_arena);
            cw_g_keybind_file = keybind_file;
            cw_g_keybind_generation = keybind_file->generation;
            cw_g_keybind_table = CW_KeybindTableMake(cw_g_keybind_arena, 1024);
            for(MD_EachNode(keybind, root->first_child))
            {
                M_Temp scratch = GetScratch(0, 0);
                
                // rjf: get cmd/key strings
                String8 cmd_name = Str8FromMD(keybind->first_child->string);
                String8 key_name = Str8FromMD(keybind->last_child->string);
                
                // rjf: find cmd kind
                CW_CmdKind cmd_kind = CW_CmdKind_Null;
                for(CW_CmdKind k = (CW_CmdKind)(CW_CmdKind_Null+1);
                    k < CW_CmdKind_COUNT;
                    k = (CW_CmdKind)(k + 1))
                {
                    if(Str8Match(cmd_name, cw_g_cmd_kind_string_table[k], MatchFlag_CaseInsensitive))
                    {
                        cmd_kind = k;
                        break;
                    }
                }
                
                // rjf: find key
                OS_Key key = OS_Key_Null;
                for(OS_Key k = (OS_Key)(OS_Key_Null + 1);
                    k < OS_Key_COUNT;
                    k = (OS_Key)(k + 1))
                {
                    String8 k_name = PushStr8Copy(scratch.arena, OS_StringFromKey(k));
                    for(U64 idx = 0; idx < k_name.size; idx += 1)
                    {
                        if(k_name.str[idx] == ' ')
                        {
                            k_name.str[idx] = '_';
                        }
                    }
                    if(Str8Match(key_name, k_name, MatchFlag_CaseInsensitive))
                    {
                        key = k;
                        break;
                    }
                }
                
                // rjf: gather modifiers
                OS_Modifiers modifiers = 0;
                for(MD_Node *modifier = keybind->first_child->next;
                    !MD_NodeIsNil(modifier) && modifier != keybind->last_child;
                    modifier = modifier->next)
                {
                    String8 modifier_name = Str8FromMD(modifier->string);
                    if(Str8Match(modifier_name, Str8Lit("alt"), MatchFlag_CaseInsensitive))
                    {
                        modifiers |= OS_Modifier_Alt;
                    }
                    else if(Str8Match(modifier_name, Str8Lit("ctrl"), MatchFlag_CaseInsensitive))
                    {
                        modifiers |= OS_Modifier_Ctrl;
                    }
                    else if(Str8Match(modifier_name, Str8Lit("shift"), MatchFlag_CaseInsensitive))
                    {
                        modifiers |= OS_Modifier_Shift;
                    }
                }
                
                // rjf: insert binding into table
                CW_Keybind keybind = CW_KeybindMake(cmd_kind, key, modifiers);
                CW_KeybindTableInsert(cw_g_keybind_arena, &cw_g_keybind_table, keybind);
                
                ReleaseScratch(scratch);
            }
        }
    }
    
    //- rjf: animate theme
    {
        // rjf: get theme values from config, & fill target theme from it
        {
            // rjf: get theme metadesk root
            MD_Node *theme_md = CFG_NodeFromKey(Str8Lit("theme"), 1);
            
            // rjf: fill colors
            for(MD_EachNode(t, theme_md->first_child))
            {
                String8 color_code_string = Str8FromMD(t->first_child->string);
                MD_Node *color_md = t->first_child->next;
                Vec4F32 color = {0};
                if(MD_ChildCountFromNode(color_md) != 0)
                {
                    color.r = (F32)F64FromStr8(Str8FromMD(color_md->first_child->string));
                    color.g = (F32)F64FromStr8(Str8FromMD(color_md->first_child->next->string));
                    color.b = (F32)F64FromStr8(Str8FromMD(color_md->first_child->next->next->string));
                    color.a = (F32)F64FromStr8(Str8FromMD(color_md->first_child->next->next->next->string));
                }
                else
                {
                    U32 val = MD_CStyleIntFromString(color_md->string);
                    color = Vec4F32FromHexRGBA(val);
                }
                if(color_code_string.size != 0)
                {
                    TM_ThemeSetColor(state->arena, &state->target_theme, color_code_string, color);
                }
            }
        }
        
        // rjf: animate to target
        for(U64 idx = 0; idx < state->target_theme.table_size; idx += 1)
        {
            for(TM_ThemeColorNode *n = state->target_theme.table[idx]; n != 0; n = n->hash_next)
            {
                TM_ThemeColorNode *theme_node = TM_ThemeColorNodeFromString(&state->theme, n->string);
                if(theme_node == 0)
                {
                    TM_ThemeSetColor(state->arena, &state->theme, n->string, V4F32(0, 0, 0, 0));
                }
                else
                {
                    APP_AnimateF32_Exp(&theme_node->color.r, n->color.r, 24.f);
                    APP_AnimateF32_Exp(&theme_node->color.g, n->color.g, 24.f);
                    APP_AnimateF32_Exp(&theme_node->color.b, n->color.b, 24.f);
                    APP_AnimateF32_Exp(&theme_node->color.a, n->color.a, 24.f);
                }
            }
        }
    }
    
    //- rjf: keybindings
    {
        // rjf: command submission
        {
            M_Temp scratch = GetScratch(0, 0);
            C_Panel *panel = state->focused_panel;
            C_View *view = CW_ViewFromPanel(panel);
            CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, view->command);
            String8 cmd_string = cmd_node->string;
            CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
            CW_ViewKind view_kind = cw_g_cmd_kind_view_kind_table[cmd_kind];
            B32 view_kind_takes_text_input = cw_g_view_kind_text_focus_table[view_kind];
            B32 panel_input_is_focused = (!view_kind_takes_text_input || panel->input_is_focused);
            
            // rjf: enter => submit command
            if(panel_input_is_focused && OS_KeyPress(events, window->handle, OS_Key_Enter, 0))
            {
                CW_PushCmd(state, panel->input);
                CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, panel->input);
                String8 cmd_string = cmd_node->string;
                CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
                B32 keep_input = cw_g_cmd_kind_keep_input_table[cmd_kind];
                if(!keep_input)
                {
                    C_PanelFillInput(panel, Str8Lit(""));
                }
                OS_TextCodepoint(events, window->handle, '\n');
            }
            
            // rjf: escape while typing in panel input => clear command
            if(panel_input_is_focused && OS_KeyPress(events, window->handle, OS_Key_Esc, 0))
            {
                C_PanelFillInput(panel, Str8Lit(""));
            }
            
            ReleaseScratch(scratch);
        }
        
        // rjf: cmd hotkeys
        state->cmd_reg.panel = state->focused_panel;
        state->cmd_reg.view = CW_ViewFromPanel(state->cmd_reg.panel);
        for(OS_Event *event = events->first, *next = 0; event != 0; event = next)
        {
            next = event->next;
            if(OS_HandleMatch(event->window, window->handle))
            {
                if(event->kind == OS_EventKind_Press)
                {
                    CW_CmdKind kind = CW_KeybindTableLookup(&cw_g_keybind_table, event->key, event->modifiers);
                    if(kind != CW_CmdKind_Null)
                    {
                        U32 codepoint = OS_CharacterFromModifiersAndKey(event->modifiers, event->key);
                        if(codepoint != 0)
                        {
                            OS_TextCodepoint(events, window->handle, codepoint);
                        }
                        OS_EatEvent(events, event);
                        CW_DoCmdFastPath(state, state->focused_panel, kind);
                    }
                }
                else if(event->kind == OS_EventKind_Text)
                {
                    OS_EatEvent(events, event);
                    text_write_codepoint = event->character;
                }
            }
        }
    }
    
    //- rjf: do commands
    {
        M_Temp scratch = GetScratch(0, 0);
        for(C_CmdNode *n = state->cmd_batch.first; n != 0; n = n->next)
        {
            // rjf: extract info for this command
            C_Cmd cmd = n->cmd;
            String8 string = cmd.string;
            C_CmdRegister reg = cmd.reg;
            CMD_Node *node = CMD_ParseNodeFromString(scratch.arena, string);
            CW_CmdKind kind = CW_CmdKindFromString(node->string);
            
            // rjf: extra parameters for panel splitting
            Axis2 split_axis = Axis2_X;
            U64 panel_sib_off = 0;
            U64 panel_child_off = 0;
            
            // rjf: extra parameters for cursor controls
            TE_InputAction action = {0};
            S64 v_dir = 0;
            
            // rjf: do command
            switch(kind)
            {
                //- rjf: unrecognized
                case CW_CmdKind_Null:
                {
                    C_PanelErrorF(reg.panel, "Unrecognized command.");
                }break;
                
                //- rjf: window creation
                case CW_CmdKind_Window:
                {
                    APP_WindowOpen(Str8Lit("[Telescope] Developer"), V2S64(1280, 720), CW_WindowUserHooks());
                }break;
                
                //- rjf: panel creation
                case CW_CmdKind_NewPanelRight: split_axis = Axis2_X; goto split;
                case CW_CmdKind_NewPanelDown:  split_axis = Axis2_Y; goto split;
                split:;
                {
                    C_Panel *panel = state->focused_panel;
                    C_Panel *parent = panel->parent;
                    if(!C_PanelIsNil(parent) && parent->split_axis == split_axis)
                    {
                        C_Panel *next = C_PanelAlloc();
                        C_PanelInsert(parent, panel, next);
                        next->pct_of_parent = 1.f / parent->child_count;
                        for(C_Panel *child = parent->first; !C_PanelIsNil(child); child = child->next)
                        {
                            if(child != next)
                            {
                                child->pct_of_parent *= (F32)(parent->child_count-1) / (parent->child_count);
                            }
                        }
                        state->focused_panel = next;
                    }
                    else
                    {
                        C_Panel *pre_prev = panel->prev;
                        C_Panel *pre_parent = parent;
                        C_Panel *new_parent = C_PanelAlloc();
                        new_parent->pct_of_parent = panel->pct_of_parent;
                        if(!C_PanelIsNil(pre_parent))
                        {
                            C_PanelRemove(pre_parent, panel);
                            C_PanelInsert(pre_parent, pre_prev, new_parent);
                        }
                        else
                        {
                            state->panel_root = new_parent;
                        }
                        
                        C_Panel *left = panel;
                        C_Panel *right = C_PanelAlloc();
                        C_PanelInsert(new_parent, &c_g_nil_panel, left);
                        C_PanelInsert(new_parent, left, right);
                        new_parent->split_axis = split_axis;
                        left->pct_of_parent = 0.5f;
                        right->pct_of_parent = 0.5f;
                        state->focused_panel = right;
                    }
                }break;
                
                //- rjf: focused panel cycling
                case CW_CmdKind_NextPanel: panel_sib_off = OffsetOf(C_Panel, next); panel_child_off = OffsetOf(C_Panel, first); goto cycle;
                case CW_CmdKind_PrevPanel: panel_sib_off = OffsetOf(C_Panel, prev); panel_child_off = OffsetOf(C_Panel, last); goto cycle;
                cycle:;
                {
                    for(C_Panel *panel = state->focused_panel; !C_PanelIsNil(panel);)
                    {
                        C_PanelRec rec = C_PanelRecDF(panel, panel_sib_off, panel_child_off);
                        panel = rec.next;
                        if(C_PanelIsNil(panel))
                        {
                            panel = state->panel_root;
                        }
                        if(C_PanelIsNil(panel->first))
                        {
                            state->next_focused_panel = panel;
                            break;
                        }
                    }
                }break;
                
                //- rjf: panel removal
                case CW_CmdKind_ClosePanel:
                {
                    C_Panel *panel = reg.panel;
                    C_Panel *parent = panel->parent;
                    if(!C_PanelIsNil(parent))
                    {
                        // NOTE(rjf): If we're removing all but the last child of this parent,
                        // we should just remove both children.
                        if(parent->child_count == 2)
                        {
                            C_Panel *discard_child = panel;
                            C_Panel *keep_child = panel == parent->first ? parent->last : parent->first;
                            C_Panel *grandparent = parent->parent;
                            C_Panel *parent_prev = parent->prev;
                            F32 pct = parent->pct_of_parent;
                            
                            // rjf: unhook children
                            C_PanelRemove(parent, keep_child);
                            C_PanelRemove(parent, discard_child);
                            
                            // rjf: unhook this subtree
                            if(!C_PanelIsNil(grandparent))
                            {
                                C_PanelRemove(grandparent, parent);
                            }
                            
                            // rjf: release the things we should discard
                            {
                                C_PanelRelease(parent);
                                C_PanelRelease(discard_child);
                            }
                            
                            // rjf: re-hook our kept child into the overall tree
                            if(C_PanelIsNil(grandparent))
                            {
                                state->panel_root = keep_child;
                            }
                            else
                            {
                                C_PanelInsert(grandparent, parent_prev, keep_child);
                            }
                            keep_child->pct_of_parent = pct;
                            
                            // rjf: reset focus, if needed
                            if(state->focused_panel == discard_child)
                            {
                                for(C_Panel *p = keep_child; !C_PanelIsNil(p); p = p->first)
                                {
                                    if(C_PanelIsNil(p->first))
                                    {
                                        state->focused_panel = p;
                                        break;
                                    }
                                }
                            }
                        }
                        // NOTE(rjf): Otherwise we can just remove this child.
                        else
                        {
                            C_Panel *next = &c_g_nil_panel;
                            if(C_PanelIsNil(next)) { next = panel->prev; }
                            if(C_PanelIsNil(next)) { next = panel->next; }
                            C_PanelRemove(parent, panel);
                            C_PanelRelease(panel);
                            if(state->focused_panel == panel)
                            {
                                for(C_Panel *p = next; !C_PanelIsNil(p); p = p->first)
                                {
                                    if(C_PanelIsNil(p->first))
                                    {
                                        state->focused_panel = p;
                                        break;
                                    }
                                }
                            }
                            for(C_Panel *child = parent->first; !C_PanelIsNil(child); child = child->next)
                            {
                                child->pct_of_parent *= (F32)(parent->child_count+1) / (parent->child_count);
                            }
                        }
                    }
                }break;
                
                //- rjf: view cycling
                case CW_CmdKind_NextView:
                {
                    C_Panel *panel = reg.panel;
                    C_View *view = panel->selected_view->next;
                    if(C_ViewIsNil(view))
                    {
                        view = panel->first_view;
                    }
                    panel->selected_view = view;
                }break;
                case CW_CmdKind_PrevView:
                {
                    C_Panel *panel = reg.panel;
                    C_View *view = panel->selected_view->prev;
                    if(C_ViewIsNil(view))
                    {
                        view = panel->last_view;
                    }
                    panel->selected_view = view;
                }break;
                case CW_CmdKind_CloseView:
                {
                    C_Panel *panel = reg.panel;
                    C_View *view = reg.view;
                    C_PanelRemoveView(panel, view);
                    C_ViewRelease(view);
                }break;
                case CW_CmdKind_EditView:
                {
                    C_Panel *panel = reg.panel;
                    C_View *view = reg.view;
                    String8 new_string = Str8SkipChopWhitespace(Str8Skip(string, node->range_in_input.max));
                    C_ViewSetCommandAndEntity(view, new_string, C_EntityFromCommandString(new_string));
                }break;
                
                //- rjf: projects
                case CW_CmdKind_OpenProject:
                {
                    String8 path = Str8SkipChopWhitespace(Str8Skip(string, node->range_in_input.max));
                    C_Entity *entity = C_OpenFile(path);
                    if(C_EntityIsNil(entity))
                    {
                        C_PanelErrorF(reg.panel, "Failed to load file.");
                    }
                    else
                    {
                        CFG_SetProjectPath(path);
                    }
                }break;
                case CW_CmdKind_CloseProject:
                {
                    CFG_SetProjectPath(Str8Lit(""));
                }break;
                
                //- rjf: system commands
                case CW_CmdKind_SystemCommand:
                {
                }break;
                
                //- rjf: stateful view commits
#if 0
                case CW_CmdKind_Code:
                {
                    C_Panel *panel = reg.panel;
                    C_View *view = C_ViewAlloc();
                    view->view_off = panel->transient_view->view_off;
                    panel->transient_view->view_off = V2F32(0, 0);
                    C_ViewSetCommandAndEntity(view, string, C_EntityFromCommandString(string));
                    C_PanelInsertView(panel, panel->last_view, view);
                    panel->selected_view = view;
                }break;
#endif
            }
        }
        M_ArenaClear(state->cmd_arena);
        MemoryZeroStruct(&state->cmd_batch);
        ReleaseScratch(scratch);
    }
    
    //- rjf: build UI
    UI_BeginBuild(state->ui_state, events, window->handle, APP_DT());
    UI_PushBackgroundColor(TM_ThemeColorFromString(&state->theme, Str8Lit("base_background")));
    UI_PushTextColor(TM_ThemeColorFromString(&state->theme, Str8Lit("base_text")));
    UI_PushBorderColor(TM_ThemeColorFromString(&state->theme, Str8Lit("base_border")));
    UI_PushFont(APP_DefaultFont());
    {
        F32 top_bar_height = 24.f;
        Rng2F32 window_rect = OS_ClientRectFromWindow(window->handle);
        Rng2F32 top_bar_rect = R2F32(V2F32(0, 0), V2F32(window_rect.x1, top_bar_height));
        Rng2F32 panel_tree_rect = R2F32(V2F32(0, top_bar_height), V2F32(window_rect.x1, window_rect.y1));
        
        //- rjf: drag/drop tooltips
        {
            if(CW_ViewDragIsActive())
            {
                C_View *view = CW_DraggedView();
                UI_Tooltip
                    UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
                    UI_BackgroundColor(TM_ThemeColorFromString(&state->theme, Str8Lit("tab_active")))
                    UI_PrefWidth(UI_TextDim(10.f, 1.f))
                {
                    UI_LabelF("%S", view->command);
                }
            }
        }
        
        //- rjf: top bar menu
        {
            UI_WindowBegin(top_bar_rect, Str8Lit("###top_bar"));
            UI_Flags(UI_BoxFlag_TextAlignCenter)
                UI_PrefWidth(UI_TextDim(8.f, 1.f))
                UI_PrefHeight(UI_Pct(1.f, 1.f))
                UI_Row
            {
                UI_ButtonF("Project");
                UI_ButtonF("Help");
                UI_Spacer(UI_Pct(1, 0));
                UI_TextColor(TM_ThemeColorFromString(&state->theme, Str8Lit("keyboard_focus")))
                    UI_PrefWidth(UI_Pixels(top_bar_height, 1.f))
                    UI_IconButton(C_Logo(), R2F32(V2F32(0, 0), APP_SizeFromTexture(C_Logo())), Str8Lit("Logo Button"));
            }
            UI_WindowEnd();
        }
        
        //- rjf: panel resize handles
        for(C_Panel *panel = state->panel_root; !C_PanelIsNil(panel);)
        {
            Rng2F32 panel_rect = C_RectFromPanel(panel_tree_rect, state->panel_root, panel);
            
            if(!C_PanelIsNil(panel->first))
            {
                Axis2 advance_axis = panel->split_axis;
                for(C_Panel *child = panel->first; !C_PanelIsNil(child->next); child = child->next)
                {
                    Rng2F32 child_rect = C_RectFromPanelChild(panel_rect, panel, child);
                    Rng2F32 handle_rect = child_rect;
                    handle_rect.p0.v[advance_axis] = handle_rect.p1.v[advance_axis];
                    handle_rect.p0.v[advance_axis] -= 2;
                    handle_rect.p1.v[advance_axis] += 2;
                    UI_Rect(handle_rect)
                    {
                        UI_Box *resize_handle = UI_BoxMake(UI_BoxFlag_Clickable, PushStr8F(APP_FrameArena(), "###%p_resize_handle", child));
                        UI_BoxEquipHoverCursor(resize_handle, advance_axis == Axis2_X ? OS_CursorKind_WestEast : OS_CursorKind_NorthSouth);
                        UI_InteractResult interact = UI_BoxInteract(resize_handle);
                        if(interact.dragging)
                        {
                            C_Panel *min_panel = child;
                            C_Panel *max_panel = child->next;
                            if(interact.pressed)
                            {
                                state->drag_boundary_min_pct_start = min_panel->pct_of_parent;
                                state->drag_boundary_max_pct_start = max_panel->pct_of_parent;
                            }
                            else
                            {
                                Vec2F32 mouse_delta      = interact.drag_delta;
                                F32 total_size           = Dim2F32(panel_rect).v[advance_axis];
                                F32 min_pct__before      = state->drag_boundary_min_pct_start;
                                F32 min_pixels__before   = min_pct__before * total_size;
                                F32 min_pixels__after    = min_pixels__before + mouse_delta.v[advance_axis];
                                F32 min_pct__after       = min_pixels__after / total_size;
                                F32 pct_delta            = min_pct__after - min_pct__before;
                                F32 max_pct__before      = state->drag_boundary_max_pct_start;
                                F32 max_pct__after       = max_pct__before - pct_delta;
                                min_panel->pct_of_parent = min_pct__after;
                                max_panel->pct_of_parent = max_pct__after;
                            }
                        }
                    }
                }
            }
            
            // rjf: iterate
            C_PanelRec rec = C_PanelRecDF_Pre(panel);
            panel = rec.next;
        }
        
        //- rjf: leaf panel contents
        for(C_Panel *panel = state->panel_root, *next = 0; !C_PanelIsNil(panel); panel = next)
        {
            C_PanelRec rec = C_PanelRecDF_Pre(panel);
            next = rec.next;
            if(!C_PanelIsNil(panel->first))
            {
                continue;
            }
            
            // rjf: grab info & begin
            Rng2F32 panel_rect = C_RectFromPanel(panel_tree_rect, state->panel_root, panel);
            panel_rect = Pad2F32(panel_rect, -1.f);
            B32 keyboard_focused = (state->focused_panel == panel);
            UI_Box *panel_box = UI_WindowBeginF(panel_rect, "###panel_%p", panel);
            
            // rjf: fill transient view inputs
            {
                C_ViewSetCommandAndEntity(panel->transient_view, panel->input, C_EntityFromCommandString(panel->input));
            }
            
            // rjf: overlay
            if(keyboard_focused)
                UI_Rect(R2F32(V2F32(0, 0), Dim2F32(panel_rect)))
                UI_BorderColor(TM_ThemeColorFromString(&state->theme, Str8Lit("keyboard_focus")))
            {
                UI_BoxMake(UI_BoxFlag_DrawBorder, Str8Lit(""));
            }
            
            // rjf: view tabs
            UI_NamedColumnF("###view_tab_group_%p", panel)
            {
                M_Temp scratch = GetScratch(0, 0);
                
                // rjf: set up info for tabs
                U64 view_tab_count = panel->view_count;
                UI_Box **view_tab_boxes = PushArrayZero(scratch.arena, UI_Box *, view_tab_count);
                C_View **view_tab_views = PushArrayZero(scratch.arena, C_View *, view_tab_count);
                
                // rjf: do tabs
                C_View *panel_active_view = CW_ViewFromPanel(panel);
                UI_Spacer(UI_Pixels(4.f, 1.f));
                UI_Row UI_PrefHeight(UI_Pixels(APP_DefaultFont().line_advance*1.2f, 1.f))
                {
                    F32 padding_between_tabs = 10.f;
                    F32 pref_tab_width = 250.f;
                    UI_Spacer(UI_Pixels(padding_between_tabs, 1.f));
                    U64 view_idx = 0;
                    for(C_View *view = panel->first_view; !C_ViewIsNil(view); view = view->next, view_idx += 1)
                        UI_PrefWidth(UI_Pixels(pref_tab_width, 0.f))
                        UI_BackgroundColor(panel_active_view == view ?
                                           TM_ThemeColorFromString(&state->theme, Str8Lit("tab_active")) : TM_ThemeColorFromString(&state->theme, Str8Lit("tab_inactive")))
                    {
                        String8 tab_name = view->command;
                        C_Entity *entity = C_EntityFromCommandString(tab_name);
                        if(!C_EntityIsNil(entity) && entity->name.size != 0)
                        {
                            tab_name = entity->name;
                        }
                        UI_Box *tab_box = CW_TabBeginF("%S##tab_%p_%p", tab_name, panel, view);
                        view_tab_boxes[view_idx] = tab_box;
                        view_tab_views[view_idx] = view;
                        
                        UI_Spacer(UI_Pct(1, 0));
                        
                        UI_PrefWidth(UI_Pixels(APP_DefaultFont().line_advance*1.2f, 1.f))
                            UI_Font(APP_IconsFont())
                            if(UI_CloseButtonF("##tab_close_%p_%p", panel, view).clicked)
                        {
                            state->cmd_reg.panel = panel;
                            state->cmd_reg.view = view;
                            CW_PushCmdF(state, "%S", cw_g_cmd_kind_string_table[CW_CmdKind_CloseView]);
                        }
                        
                        UI_InteractResult tab_itrct = CW_TabEnd();
                        if(tab_itrct.dragging)
                        {
                            if(tab_itrct.pressed)
                            {
                                panel->selected_view = view;
                            }
                            if(Length2F32(tab_itrct.drag_delta) > 50.f)
                            {
                                CW_ViewDragBegin(panel, view);
                            }
                        }
                        
                        UI_Spacer(UI_Pixels(padding_between_tabs, 1.f));
                    }
                }
                
                // rjf: detect which tab-inbetween-spot the mouse is closest to
                C_View *nearest_prev_view = &c_g_nil_view;
                Vec2F32 nearest_center = {0};
                if(view_tab_count != 0)
                {
                    typedef struct Map Map;
                    struct Map
                    {
                        C_View *prev_view;
                        Vec2F32 center;
                    };
                    Map *center_prev_view_map = PushArrayZero(scratch.arena, Map, view_tab_count+1);
                    UI_Box *first_box = view_tab_boxes[0];
                    center_prev_view_map[0].prev_view = &c_g_nil_view;
                    center_prev_view_map[0].center = V2F32(first_box->rect.x0, (first_box->rect.y0+first_box->rect.y1)/2);
                    for(U64 view_idx = 0; view_idx < view_tab_count; view_idx += 1)
                    {
                        UI_Box *box = view_tab_boxes[view_idx];
                        center_prev_view_map[view_idx+1].prev_view = view_tab_views[view_idx];
                        center_prev_view_map[view_idx+1].center = V2F32(box->rect.x1, (box->rect.y0+box->rect.y1)/2);
                    }
                    
                    Vec2F32 mouse = OS_MouseFromWindow(window->handle);
                    F32 best_distance = -1;
                    for(U64 idx = 0; idx < view_tab_count+1; idx += 1)
                    {
                        Vec2F32 center = center_prev_view_map[idx].center;
                        F32 distance = Length2F32(Sub2F32(center, mouse));
                        if(best_distance < 0 || distance < best_distance)
                        {
                            best_distance = distance;
                            nearest_prev_view = center_prev_view_map[idx].prev_view;
                            nearest_center = center;
                        }
                    }
                }
                
                // rjf: view drop
                {
                    UI_Box *group_box = UI_ActiveParent();
                    C_View *view = CW_DraggedView();
                    
                    // rjf: (more precise)
                    if(!C_ViewIsNil(view) &&
                       Contains2F32(group_box->rect, OS_MouseFromWindow(window->handle)))
                    {
                        DR_Bucket bucket = {0};
                        DR_Rect(&bucket, group_box->rect, TM_ThemeColorFromString(&state->theme, Str8Lit("drop_site")));
                        DR_Rect(&bucket, R2F32(Sub2F32(nearest_center, V2F32(+2.f, +10.f)),
                                               Add2F32(nearest_center, V2F32(+2.f, +10.f))),
                                V4F32(1, 1, 1, 1));
                        UI_BoxEquipDrawBucket(panel_box, &bucket);
                        if(!UI_DragIsActive())
                        {
                            CW_ViewDragEnd(panel, nearest_prev_view);
                        }
                    }
                    
                    // rjf: (less precise)
                    else if(!C_ViewIsNil(view) &&
                            Contains2F32(panel_box->rect, OS_MouseFromWindow(window->handle)))
                    {
                        DR_Bucket bucket = {0};
                        DR_Rect(&bucket, panel_box->rect, TM_ThemeColorFromString(&state->theme, Str8Lit("drop_site")));
                        UI_BoxEquipDrawBucket(panel_box, &bucket);
                        if(!UI_DragIsActive())
                        {
                            CW_ViewDragEnd(panel, panel->last_view);
                        }
                    }
                }
                
                ReleaseScratch(scratch);
            }
            
            // rjf: panel input
            UI_InteractResult input_itrct = {0};
            {
                M_Temp scratch = GetScratch(0, 0);
                C_View *view = CW_ViewFromPanel(panel);
                CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, view->command);
                String8 cmd_string = cmd_node->string;
                CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
                CW_ViewKind view_kind = cw_g_cmd_kind_view_kind_table[cmd_kind];
                B32 view_kind_takes_text_input = cw_g_view_kind_text_focus_table[view_kind];
                B32 panel_input_is_focused = keyboard_focused && (!view_kind_takes_text_input || panel->input_is_focused);
                
                // rjf: f1 => focus or unfocus panel command input
                if(keyboard_focused && view_kind_takes_text_input)
                {
                    if(OS_KeyPress(events, window->handle, OS_Key_F1, 0))
                    {
                        panel->input_is_focused = !panel->input_is_focused;
                    }
                }
                
                // rjf: apply text ops
                if(panel_input_is_focused)
                {
                    for(TE_InputActionNode *n = text_actions.actions.first; n != 0; n = n->next)
                    {
                        TE_InputAction action = n->action;
                        TE_Op op = TE_SingleLineOpFromInputAction(scratch.arena, action, panel->input, panel->cursor, panel->mark);
                        if(op.copy != 0 && !TE_PointMatch(panel->cursor, panel->mark))
                        {
                            OS_SetClipboardText(Substr8(panel->input, R1U64(panel->cursor.column-1, panel->mark.column-1)));
                        }
                        if(op.paste != 0)
                        {
                            op.replace = OS_GetClipboardText(scratch.arena);
                            op.cursor = op.mark = TE_PointMake(1, op.cursor.column + op.replace.size);
                        }
                        Rng1S64 op_range = R1S64(op.range.min.column-1, op.range.max.column-1);
                        String8 new_string = TE_ReplacedRangeStringFromString(scratch.arena, panel->input, op_range, op.replace);
                        new_string.size = Min(sizeof(panel->input_buffer), new_string.size);
                        panel->cursor = op.cursor;
                        panel->mark = op.mark;
                        MemoryCopy(panel->input.str, new_string.str, new_string.size);
                        panel->input.size = new_string.size;
                        UI_ResetCaretBlinkT();
                    }
                    MemoryZeroStruct(&text_actions);
                }
                
                // rjf: form message for when the input is empty
                String8 empty_msg = {0};
                if(!panel_input_is_focused && view_kind_takes_text_input)
                {
                    empty_msg = Str8Lit("F1");
                    UI_PushTextColor(TM_ThemeColorFromString(&state->theme, Str8Lit("base_text_weak")));
                }
                
                // rjf: do ui
                input_itrct = CW_LineEditF(panel_input_is_focused, &panel->cursor, &panel->mark, sizeof(panel->input_buffer), &panel->input, "%S###panel_input_%p", empty_msg, panel);
                
                // rjf: pop text color if we pushed one for the empty message
                if(!panel_input_is_focused && view_kind_takes_text_input)
                {
                    UI_PopTextColor();
                }
                
                ReleaseScratch(scratch);
            }
            
            // rjf: syntax highlighting for panel input
            {
                M_Temp scratch = GetScratch(0, 0);
                CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, panel->input);
                String8 cmd_string = cmd_node->string;
                CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
                if(cmd_kind != CW_CmdKind_Null)
                {
                    UI_Box *box = input_itrct.box;
                    String8 command = PushStr8Copy(UI_FrameArena(), panel->input);
                    Vec4F32 normal_color = box->text_color;
                    Vec4F32 keyword_color = TM_ThemeColorFromString(&state->theme, Str8Lit("code_keyword"));
                    box->flags &= ~UI_BoxFlag_DrawText;
                    box->flags |= UI_BoxFlag_DrawFancyText;
                    
                    FancyStringList strs = {0};
                    if(cmd_node->range_in_input.min > 0)
                    {
                        FancyStringListPush(UI_FrameArena(), &strs, FancyStringMake(normal_color, Prefix8(command, cmd_node->range_in_input.min)));
                    }
                    FancyStringListPush(UI_FrameArena(), &strs, FancyStringMake(keyword_color, Substr8(command, cmd_node->range_in_input)));
                    if(cmd_node->range_in_input.max < command.size)
                    {
                        FancyStringListPush(UI_FrameArena(), &strs, FancyStringMake(normal_color, Str8Skip(command, cmd_node->range_in_input.max)));
                    }
                    UI_BoxEquipFancyStringList(box, strs);
                }
                ReleaseScratch(scratch);
            }
            
            // rjf: change focused panel on input interact
            if(input_itrct.pressed)
            {
                state->next_focused_panel = panel;
            }
            
            // rjf: error message
            {
                UI_PushParent(input_itrct.box);
                if(panel->error_t > 0.f)
                {
                    panel->error_t -= APP_DT()/8;
                    UI_Spacer(UI_Pct(1, 0));
                    UI_PrefWidth(UI_ChildrenSum(1)) UI_Column
                    {
                        UI_Spacer(UI_Pixels(2.f, 1.f));
                        UI_Flags(UI_BoxFlag_TextAlignCenter)
                            UI_TextColor(V4F32(1, 1, 1, panel->error_t))
                            UI_PrefHeight(UI_Pct(1, 0))
                            UI_CornerRadius(4.f)
                        {
                            UI_InteractResult itrct = {0};
                            UI_Flags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder)
                                UI_BackgroundColor(V4F32(0.6f, 0.05f, 0.01f, panel->error_t))
                                UI_BorderColor(V4F32(1, 1, 1, 0.2f*panel->error_t))
                                UI_PrefWidth(UI_ChildrenSum(1))
                            {
                                itrct = UI_Label(Str8Lit(""));
                            }
                            
                            UI_PushParent(itrct.box);
                            UI_PrefWidth(UI_TextDim(8, 1))
                            {
                                UI_Font(APP_IconsFont()) UI_LabelF("i");
                                UI_Label(panel->error);
                            }
                            UI_PopParent();
                            
                        }
                        UI_Spacer(UI_Pixels(2.f, 1.f));
                    }
                    UI_Spacer(UI_Pixels(2.f, 1.f));
                }
                UI_PopParent();
            }
            
            // TODO(rjf): test plugins
            {
                M_Temp scratch = GetScratch(0, 0);
                C_View *view = CW_ViewFromPanel(panel);
                CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, view->command);
                String8 cmd_string = cmd_node->string;
                C_Plugin *plugin = C_PluginFromString(cmd_string);
                if(plugin != 0)
                {
                    view->plugin_state = plugin->TickView(view->arena, view->plugin_state, window, events, 0, panel_rect);
                }
            }
            
            // rjf: view
            {
                M_Temp scratch = GetScratch(0, 0);
                C_View *view = CW_ViewFromPanel(panel);
                CMD_Node *cmd_node = CMD_ParseNodeFromString(scratch.arena, view->command);
                String8 cmd_string = cmd_node->string;
                CW_CmdKind cmd_kind = CW_CmdKindFromString(cmd_string);
                CW_ViewKind view_kind = cw_g_cmd_kind_view_kind_table[cmd_kind];
                CW_ViewFunction *view_function = cw_g_view_kind_function_table[view_kind];
                
                // rjf: open scrollable parent
                UI_WidthFill UI_HeightFill UI_ViewOffStorage(&view->view_off)
                {
                    UI_ScrollableParentBeginF(0, 1, "###panel_content_%p", panel);
                }
                
                // rjf: do view ui
                view_function(keyboard_focused, state, text_actions.actions, panel, view, view->command, cmd_node, UI_ActiveParent());
                if(keyboard_focused)
                {
                    MemoryZeroStruct(&text_actions);
                }
                
                // rjf: store content box
                panel->content_box = UI_ActiveParent();
                
                // rjf: close scrollable parent
                UI_ScrollableParentEnd();
                
                ReleaseScratch(scratch);
            }
            
            // rjf: end
            UI_WindowEnd();
            
            // rjf: clicking blank area of panel => focus it
            UI_InteractResult panel_interact = UI_BoxInteract(panel_box);
            if(panel_interact.pressed)
            {
                state->next_focused_panel = panel;
            }
        }
        
    }
    UI_EndBuild();
    
    //- rjf: render UI
    DR_Bucket bucket = {0};
    for(UI_Box *box = UI_RootFromState(state->ui_state), *next = 0; !UI_BoxIsNil(box); box = next)
    {
        UI_BoxRec rec = UI_BoxRecDF_Post(box);
        next = rec.next;
        
        // rjf: background
        if(box->flags & UI_BoxFlag_DrawBackground)
        {
            DR_Rect_C(&bucket, box->rect, box->background_color, box->corner_radius);
            if(box->flags & UI_BoxFlag_DrawHotEffects)
            {
                DR_Rect_VCB(&bucket, box->rect,
                            V4F32(1, 1, 1, 0.2f*box->hot_t),
                            V4F32(1, 1, 1, 0.0f),
                            V4F32(1, 1, 1, 0.2f*box->hot_t),
                            V4F32(1, 1, 1, 0.0f),
                            box->corner_radius, 0);
            }
            if(box->flags & UI_BoxFlag_DrawActiveEffects)
            {
                DR_Rect_VCB(&bucket, box->rect,
                            V4F32(0, 0, 0, 0.4f*box->active_t),
                            V4F32(0, 0, 0, 0.0f),
                            V4F32(0, 0, 0, 0.4f*box->active_t),
                            V4F32(0, 0, 0, 0.0f),
                            box->corner_radius, 0);
                DR_Rect_VCB(&bucket,
                            R2F32(V2F32(box->rect.x0, box->rect.y0),
                                  V2F32(box->rect.x1, box->rect.y0+(box->rect.y1-box->rect.y0)*(box->active_t))),
                            V4F32(0, 0, 0, 0.7f*box->active_t),
                            V4F32(0, 0, 0, 0.0f),
                            V4F32(0, 0, 0, 0.7f*box->active_t),
                            V4F32(0, 0, 0, 0.0f),
                            box->corner_radius, 0);
                DR_Rect_VCB(&bucket,
                            R2F32(V2F32(box->rect.x0, box->rect.y0),
                                  V2F32(box->rect.x0+(box->rect.x1-box->rect.x0)*(box->active_t*0.25f), box->rect.y1)),
                            V4F32(0, 0, 0, 0.5f*box->active_t),
                            V4F32(0, 0, 0, 0.5f*box->active_t),
                            V4F32(0, 0, 0, 0.0f),
                            V4F32(0, 0, 0, 0.0f),
                            box->corner_radius, 0);
                DR_Rect_VCB(&bucket,
                            R2F32(V2F32(box->rect.x1-(box->rect.x1-box->rect.x0)*(box->active_t*0.25f), box->rect.y0),
                                  V2F32(box->rect.x1, box->rect.y1)),
                            V4F32(0, 0, 0, 0.0f),
                            V4F32(0, 0, 0, 0.0f),
                            V4F32(0, 0, 0, 0.5f*box->active_t),
                            V4F32(0, 0, 0, 0.5f*box->active_t),
                            box->corner_radius, 0);
            }
        }
        
        // rjf: text
        if(box->flags & UI_BoxFlag_DrawText)
        {
            Vec2F32 text_pos = UI_BoxTextPosition(box);
            if(box->flags & UI_BoxFlag_DrawActiveEffects)
            {
                text_pos.y += 1.f*box->active_t;
            }
            String8 string = UI_DisplayStringFromBox(box);
            DR_Text(&bucket, box->text_color, text_pos, box->font, string);
        }
        
        // rjf: fancy text
        if(box->flags & UI_BoxFlag_DrawFancyText)
        {
            Vec2F32 text_pos = UI_BoxTextPosition(box);
            if(box->flags & UI_BoxFlag_DrawActiveEffects)
            {
                text_pos.y += 1.f*box->active_t;
            }
            for(FancyStringNode *n = box->fancy_strings.first; n != 0; n = n->next)
            {
                Vec4F32 color = n->string.color;
                String8 string = n->string.string;
                Vec2F32 advance = R_AdvanceFromString(box->font, string);
                DR_Text(&bucket, color, text_pos, box->font, string);
                text_pos.x += advance.x;
            }
        }
        
        // rjf: texture
        if(box->flags & UI_BoxFlag_DrawTexture)
        {
            Vec4F32 tint = V4F32(1, 1, 1, 1);
            Rng2F32 rect = box->rect;
            if(box->flags & UI_BoxFlag_DrawHotEffects)
            {
                tint = V4F32(1, 1, 1, 0.7f+0.3f*box->hot_t);
                rect = Pad2F32(rect, -2.f + 2.f*box->hot_t);
            }
            if(box->flags & UI_BoxFlag_DrawActiveEffects)
            {
                rect = Pad2F32(rect, -2.f*box->active_t);
            }
            DR_Sprite(&bucket, tint, rect, box->texture_src, box->texture);
        }
        
        // rjf: border
        if(box->flags & UI_BoxFlag_DrawBorder)
        {
            DR_Rect_CB(&bucket, box->rect, box->border_color, box->corner_radius, 2.f);
        }
        
        // rjf: custom rendering bucket
        if(box->flags & UI_BoxFlag_DrawBucket)
        {
            DR_CmdList(&bucket, box->draw_bucket.cmds);
        }
        
        // rjf: position-relative custom rendering bucket
        if(box->flags & UI_BoxFlag_DrawBucketRelative)
        {
            DR_CmdList(&bucket, box->draw_bucket_relative.cmds);
        }
        
    }
    DR_Submit(window->window_equip, bucket.cmds);
    
    //- rjf: end frame
    {
        if(!UI_DragIsActive())
        {
            CW_ViewDragEnd(&c_g_nil_panel, &c_g_nil_view);
        }
    }
}
