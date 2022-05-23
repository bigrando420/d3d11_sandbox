////////////////////////////////
//~ rjf: Widget Calls

//- rjf: basic widgets

engine_function UI_InteractResult
UI_Spacer(UI_Size size)
{
    UI_Box *parent = UI_ActiveParent();
    if(parent->child_layout_axis == Axis2_X)
    {
        UI_PushPrefWidth(size);
    }
    else
    {
        UI_PushPrefHeight(size);
    }
    UI_Box *box = UI_BoxMake(0, Str8Lit(""));
    UI_InteractResult interact = UI_BoxInteract(box);
    if(parent->child_layout_axis == Axis2_X)
    {
        UI_PopPrefWidth();
    }
    else
    {
        UI_PopPrefHeight();
    }
    return interact;
}

engine_function UI_InteractResult
UI_Label(String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_DrawText, Str8Lit(""));
    UI_BoxEquipString(box, string);
    UI_InteractResult interact = UI_BoxInteract(box);
    return interact;
}

engine_function UI_InteractResult
UI_LabelF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_Label(string);
    ReleaseScratch(scratch);
    return itrct;
}

engine_function UI_InteractResult
UI_Button(String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|
                             UI_BoxFlag_DrawBackground|
                             UI_BoxFlag_DrawBorder|
                             UI_BoxFlag_DrawText|
                             UI_BoxFlag_DrawHotEffects|
                             UI_BoxFlag_DrawActiveEffects,
                             string);
    UI_BoxEquipHoverCursor(box, OS_CursorKind_Hand);
    UI_InteractResult interact = UI_BoxInteract(box);
    return interact;
}

engine_function UI_InteractResult
UI_ButtonF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_Button(string);
    ReleaseScratch(scratch);
    return itrct;
}

#if 0
engine_function UI_InteractResult
UI_LineEdit(B32 keyboard_focused, TE_Point *cursor, TE_Point *mark, U64 string_max, String8 *out_string, String8 string)
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
    
    //- rjf: keyboard interaction
    if(keyboard_focused)
    {
        M_Temp scratch = GetScratch(0, 0);
        OS_EventList *events = UI_Events();
        for(OS_Event *event = events->first, *next = 0; event != 0; event = next)
        {
            next = event->next;
            if(!OS_HandleMatch(event->window, UI_Window()))
            {
                continue;
            }
            TE_InputAction *action = TE_InputActionFromEvent(event);
            
            //- rjf: keyboard navigation & stuff
            if(event->kind == OS_EventKind_Press && action != 0)
            {
                TE_Op op = TE_LocalSingleLineOpFromInputAction(scratch.arena, action, *out_string, *cursor, *mark);
                Rng1S64 op_range = R1S64(op.range.min.column-1, op.range.max.column-1);
                String8 new_string = TE_ReplacedRangeStringFromString(scratch.arena, *out_string, op_range, op.replace);
                new_string.size = Min(string_max, new_string.size);
                *cursor = op.cursor;
                *mark = op.mark;
                MemoryCopy(out_string->str, new_string.str, new_string.size);
                out_string->size = new_string.size;
                OS_EatEvent(events, event);
                UI_ResetCaretBlinkT();
            }
            
            //- rjf: text input
            if(event->kind == OS_EventKind_Text && event->character != '\n' && event->character != '\t')
            {
                OS_EatEvent(events, event);
                U8 written_utf8[4] = {0};
                U32 bytes = Utf8FromCodepoint(written_utf8, event->character);
                if(out_string->size + bytes <= string_max)
                {
                    String8 insert_string = Str8(written_utf8, (U64)bytes);
                    String8 new_string = TE_ReplacedRangeStringFromString(scratch.arena, *out_string, R1S64(cursor->column-1, mark->column-1), insert_string);
                    MemoryCopy(out_string->str, new_string.str, new_string.size);
                    out_string->size = new_string.size;
                    *cursor = *mark = TE_PointMake(1, Min(cursor->column, mark->column)+insert_string.size);
                }
                UI_ResetCaretBlinkT();
            }
        }
        ReleaseScratch(scratch);
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

engine_function UI_InteractResult
UI_LineEditF(B32 keyboard_focused, TE_Point *cursor, TE_Point *mark, U64 string_max, String8 *out_string, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_LineEdit(keyboard_focused, cursor, mark, string_max, out_string, string);
    ReleaseScratch(scratch);
    return itrct;
}
#endif

//- rjf: layout widgets

engine_function UI_InteractResult UI_RowBegin(void)    { return UI_NamedRowBegin(Str8Lit("")); }
engine_function void              UI_RowEnd(void)      { UI_NamedRowEnd(); }
engine_function UI_InteractResult UI_ColumnBegin(void) { return UI_NamedColumnBegin(Str8Lit("")); }
engine_function void              UI_ColumnEnd(void)   { UI_NamedColumnEnd(); }

engine_function UI_InteractResult
UI_NamedRowBegin(String8 string)
{
    UI_PushPrefWidth(UI_ChildrenSum(1.f));
    UI_PushPrefHeight(UI_ChildrenSum(1.f));
    UI_Box *box = UI_BoxMake(0, string);
    UI_BoxEquipChildLayoutAxis(box, Axis2_X);
    UI_PopPrefWidth();
    UI_PopPrefHeight();
    UI_PushParent(box);
    UI_InteractResult result = UI_BoxInteract(box);
    return result;
}

engine_function UI_InteractResult
UI_NamedRowBeginF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_NamedRowBegin(string);
    ReleaseScratch(scratch);
    return itrct;
}

engine_function void
UI_NamedRowEnd(void)
{
    UI_PopParent();
}

engine_function UI_InteractResult
UI_NamedColumnBegin(String8 string)
{
    UI_PushPrefWidth(UI_ChildrenSum(1.f));
    UI_PushPrefHeight(UI_ChildrenSum(1.f));
    UI_Box *box = UI_BoxMake(0, string);
    UI_BoxEquipChildLayoutAxis(box, Axis2_Y);
    UI_PopPrefHeight();
    UI_PopPrefWidth();
    UI_PushParent(box);
    UI_InteractResult result = UI_BoxInteract(box);
    return result;
}

engine_function UI_InteractResult
UI_NamedColumnBeginF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_NamedColumnBegin(string);
    ReleaseScratch(scratch);
    return itrct;
}

engine_function void
UI_NamedColumnEnd(void)
{
    UI_PopParent();
}

engine_function UI_InteractResult
UI_ScrollableParentBegin(B32 x, B32 y, String8 string)
{
    // rjf: first make column parent (for horizontal scroll bar)
    UI_Box *column_parent = UI_BoxMake(0, PushStr8F(UI_FrameArena(), "###_%S_column", string));
    UI_BoxEquipChildLayoutAxis(column_parent, Axis2_Y);
    UI_InteractResult column_interact = UI_BoxInteract(column_parent);
    UI_PushParent(column_parent);
    
    // rjf: then make row parent (for vertical scroll bar)
    UI_Box *row_parent = UI_BoxMake(0, PushStr8F(UI_FrameArena(), "###_%S_row", string));
    UI_BoxEquipChildLayoutAxis(row_parent, Axis2_X);
    UI_InteractResult row_interact = UI_BoxInteract(row_parent);
    UI_PushParent(row_parent);
    
    // rjf: then make expandable overflowed parent
    UI_PushPrefWidth(UI_Pct(1, 0));
    UI_PushPrefHeight(UI_Pct(1, 0));
    UI_BoxFlags flags = 0;
    if(x) { flags |= UI_BoxFlag_AllowOverflowX; }
    if(y) { flags |= UI_BoxFlag_AllowOverflowY; }
    UI_Box *main_parent = UI_BoxMake(UI_BoxFlag_ViewScroll|UI_BoxFlag_Clip|flags, string);
    UI_BoxEquipChildLayoutAxis(main_parent, Axis2_Y);
    UI_InteractResult interact = UI_BoxInteract(main_parent);
    UI_PopPrefWidth();
    UI_PopPrefHeight();
    UI_PushParent(main_parent);
    
    return interact;
}

engine_function UI_InteractResult
UI_ScrollableParentBeginF(B32 x, B32 y, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_ScrollableParentBegin(x, y, string);
    ReleaseScratch(scratch);
    return itrct;
}

engine_function void
UI_ScrollableParentEnd(void)
{
    UI_Box *main_parent = UI_PopParent();
    if(main_parent->flags & UI_BoxFlag_AllowOverflowY)
    {
        UI_ScrollBar(Axis2_Y, main_parent);
    }
    UI_PopParent();
    // TODO(rjf): make little corner so that the bottom scroll bar does not go
    // under the side scroll bar
    if(main_parent->flags & UI_BoxFlag_AllowOverflowX)
    {
        UI_ScrollBar(Axis2_X, main_parent);
    }
    UI_PopParent();
}

//- rjf: containers

engine_function UI_Box *
UI_WindowBegin(Rng2F32 rect, String8 string)
{
    UI_PushRect(rect);
    
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, string);
    UI_BoxEquipChildLayoutAxis(box, Axis2_Y);
    
    UI_PopRect();
    
    UI_PushParent(box);
    UI_PushPrefWidth(UI_Pct(1, 0));
    return box;
}

engine_function UI_Box *
UI_WindowBeginF(Rng2F32 rect, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_Box *box = UI_WindowBegin(rect, string);
    ReleaseScratch(scratch);
    return box;
}

engine_function void
UI_WindowEnd(void)
{
    UI_PopPrefWidth();
    UI_PopParent();
}

engine_function UI_InteractResult
UI_TitleBar(String8 string)
{
    UI_PushPrefWidth(UI_Pct(1.f, 1.f));
    UI_PushPrefHeight(UI_Pixels(30.f, 1.f));
    UI_Box *row = UI_BoxMake(UI_BoxFlag_Clickable|UI_BoxFlag_DrawBorder, string);
    UI_BoxEquipChildLayoutAxis(row, Axis2_X);
    UI_PushParent(row);
    UI_WidthFill UI_Label(string);
    UI_PrefWidth(UI_Pixels(35.f, 1.f)) UI_ButtonF("X");
    UI_PopParent();
    UI_PopPrefWidth();
    UI_PopPrefHeight();
    
    UI_InteractResult bar_interact = UI_BoxInteract(row);
    if(bar_interact.dragging)
    {
        UI_Box *parent = UI_ActiveParent();
        (void)parent;
    }
    
    return bar_interact;
}

engine_function void
UI_ScrollBar(Axis2 axis, UI_Box *scrolled_box)
{
    switch(axis)
    {
        default:
        case Axis2_Y:{UI_ColumnBegin();}break;
        case Axis2_X:{UI_RowBegin();}break;
    }
    
    UI_PrefWidth(UI_Pixels(20.f, 1.f))
        UI_PrefHeight(UI_Pixels(20.f, 1.f))
        UI_Flags(UI_BoxFlag_DrawBorder)
    {
        F32 src_v_size       = scrolled_box->fixed_size.v[axis];
        F32 src_v_top        = scrolled_box->view_off.v[axis];
        F32 src_v_bottom_max = scrolled_box->view_bounds.v[axis];
        
        // rjf: scroll up button
        UI_Font(APP_IconsFont())
            UI_Flags(UI_BoxFlag_TextAlignCenter)
            if(UI_ButtonF("u##_up_scroll_%p_%i", scrolled_box, axis).dragging)
        {
            scrolled_box->target_view_off.v[axis] -= 10;
        }
        
        // rjf: main scroll area
        UI_PushPrefSize(axis, UI_Pct(1.f, 0.f));
        UI_Box *scroll_area_box = UI_BoxMake(0, PushStr8F(UI_FrameArena(), "##_scroll_area_%p_%i", scrolled_box, axis));
        UI_BoxEquipChildLayoutAxis(scroll_area_box, axis);
        UI_PopPrefSize(axis);
        UI_PushParent(scroll_area_box);
        UI_PushPrefSize(!axis, UI_Pct(1.f, 0.f));
        UI_InteractResult scroll_area_interact = UI_BoxInteract(scroll_area_box);
        (void)scroll_area_interact;
        {
            F32 scroll_area_height = scroll_area_box->fixed_size.v[axis];
            F32 space_before_scroller = scroll_area_height * src_v_top / src_v_bottom_max;
            F32 scroller_size = scroll_area_height * src_v_size / src_v_bottom_max;
            UI_Spacer(UI_Pixels(space_before_scroller, 1.f));
            UI_PushPrefSize(axis, UI_Pixels(scroller_size, 1.f));
            {
                UI_InteractResult scroller_interact = UI_ButtonF("##_scroller_%p_%i", scrolled_box, axis);
                if(scroller_interact.dragging)
                {
                    if(scroller_interact.pressed)
                    {
                        Vec2F32 drag_start = {0};
                        drag_start.v[axis] = space_before_scroller;
                        UI_StoreDragData_Vec2F32(drag_start);
                    }
                    F32 pre_drag_space = UI_GetDragData_Vec2F32().v[axis];
                    F32 post_drag_space = pre_drag_space + scroller_interact.drag_delta.v[axis];
                    F32 post_drag_view_off = (post_drag_space / scroll_area_height) * src_v_bottom_max;
                    scrolled_box->target_view_off.v[axis] = post_drag_view_off;
                }
            }
            UI_PopPrefSize(axis);
            UI_Spacer(UI_Pct(1.f, 0.f));
        }
        UI_PopPrefSize(!axis);
        UI_PopParent();
        
        // rjf: scroll down button
        UI_Font(APP_IconsFont())
            UI_Flags(UI_BoxFlag_TextAlignCenter)
            if(UI_ButtonF("d##_down_scroll_%p_%i", scrolled_box, axis).dragging)
        {
            scrolled_box->target_view_off.v[axis] += 10;
        }
    }
    
    switch(axis)
    {
        default:
        case Axis2_Y:{UI_ColumnEnd();}break;
        case Axis2_X:{UI_RowEnd();}break;
    }
}

//- rjf: special buttons

engine_function UI_InteractResult
UI_CloseButton(String8 string)
{
    UI_PushBackgroundColor(V4F32(0.6f, 0.01f, 0.005f, 1.f));
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|
                             UI_BoxFlag_DrawBackground|
                             UI_BoxFlag_DrawBorder|
                             UI_BoxFlag_DrawText|
                             UI_BoxFlag_DrawHotEffects|
                             UI_BoxFlag_DrawActiveEffects|
                             UI_BoxFlag_TextAlignCenter,
                             string);
    UI_PopBackgroundColor();
    UI_BoxEquipHoverCursor(box, OS_CursorKind_Hand);
    UI_BoxEquipString(box, Str8Lit("x"));
    UI_InteractResult interact = UI_BoxInteract(box);
    return interact;
}

engine_function UI_InteractResult
UI_CloseButtonF(char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_CloseButton(string);
    ReleaseScratch(scratch);
    return itrct;
}

engine_function UI_InteractResult
UI_IconButton(R_Handle texture, Rng2F32 src, String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_Clickable|
                             UI_BoxFlag_DrawTexture|
                             UI_BoxFlag_DrawHotEffects|
                             UI_BoxFlag_DrawActiveEffects,
                             string);
    UI_BoxEquipHoverCursor(box, OS_CursorKind_Hand);
    UI_BoxEquipTexture(box, texture, src);
    UI_InteractResult interact = UI_BoxInteract(box);
    return interact;
}

engine_function UI_InteractResult
UI_IconButtonF(R_Handle texture, Rng2F32 src, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = UI_IconButton(texture, src, string);
    ReleaseScratch(scratch);
    return itrct;
}

//- rjf: miscellaneous widgets

engine_function UI_InteractResult
UI_SaturationValuePicker(F32 hue, F32 *saturation, F32 *value, String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_DrawBorder|UI_BoxFlag_Clickable, string);
    
    // rjf: get color info
    Vec3F32 hue_rgb = RGBFromHSV(V3F32(hue, 1, 1));
    
    // rjf: interaction
    UI_InteractResult interact = UI_BoxInteract(box);
    if(interact.dragging)
    {
        Vec2F32 dest_sv = Div2F32(Sub2F32(interact.mouse, box->rect.p0), Dim2F32(box->rect));
        dest_sv.x = Clamp(0, dest_sv.x, 1);
        dest_sv.y = 1 - Clamp(0, dest_sv.y, 1);
        *saturation = dest_sv.x;
        *value = dest_sv.y;
        Vec3F32 hsv = V3F32(hue, *saturation, *value);
        Vec3F32 rgb = RGBFromHSV(hsv);
        
        UI_Tooltip
            UI_PrefWidth(UI_ChildrenSum(1))
            UI_PrefHeight(UI_ChildrenSum(1))
        {
            F32 padding = 10.f;
            UI_Box *container = UI_BoxMake(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, Str8Lit(""));
            UI_BoxEquipChildLayoutAxis(container, Axis2_Y);
            UI_PushParent(container);
            {
                UI_Spacer(UI_Pixels(padding, 1.f));
                UI_Row
                {
                    UI_Spacer(UI_Pixels(padding, 1.f));
                    
                    // rjf: color preview
                    {
                        UI_PrefHeight(UI_Pixels(30.f, 1.f))
                            UI_BackgroundColor(V4F32(rgb.r, rgb.g, rgb.b, 1))
                            UI_CornerRadius(8.f)
                            UI_Flags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder)
                            UI_Spacer(UI_Pixels(30.f, 1.f));
                    }
                    
                    UI_Spacer(UI_Pixels(padding, 1.f));
                    
                    // rjf: breakdown by component
                    UI_PrefHeight(UI_ChildrenSum(1)) 
                        UI_Column
                        UI_PrefWidth(UI_TextDim(10, 1))
                        UI_PrefHeight(UI_TextDim(0, 1))
                    {
                        UI_LabelF("Red:   %.2f", rgb.r);
                        UI_LabelF("Green: %.2f", rgb.g);
                        UI_LabelF("Blue:  %.2f", rgb.b);
                        UI_Spacer(UI_Pixels(10.f, 1));
                        UI_LabelF("Hue:   %.2f", hsv.x);
                        UI_LabelF("Sat:   %.2f", hsv.y);
                        UI_LabelF("Val:   %.2f", hsv.z);
                    }
                    
                    UI_Spacer(UI_Pixels(padding, 1.f));
                }
                UI_Spacer(UI_Pixels(padding, 1.f));
            }
            UI_PopParent();
        }
    }
    
    // rjf: rendering
    DR_Bucket drb = {0};
    {
        DR_Rect_VCB(&drb, box->rect,
                    V4F32(0, 0, 0, 0),
                    V4F32(0, 0, 0, 0),
                    V4F32(0, 0, 0, 0),
                    V4F32(0, 0, 0, 0),
                    0.f, 0.f);
        DR_Rect_VCB(&drb, box->rect,
                    V4F32(1, 1, 1, 1),
                    V4F32(1, 1, 1, 0),
                    V4F32(hue_rgb.r, hue_rgb.g, hue_rgb.b, 1),
                    V4F32(hue_rgb.r, hue_rgb.g, hue_rgb.b, 0),
                    0.f, 0.f);
        Vec2F32 dim = Dim2F32(box->rect);
        Vec2F32 indicator_pct = {*saturation, 1-*value};
        Vec2F32 indicator_pos = {box->rect.x0 + indicator_pct.x*dim.x, box->rect.y0 + indicator_pct.y*dim.y};
        Vec2F32 indicator_size = {5, 5};
        DR_Rect_CB(&drb, R2F32(Sub2F32(indicator_pos, indicator_size), Add2F32(indicator_pos, indicator_size)),
                   V4F32(1, 1, 1, 1), 0, 2.f);
    }
    
    // rjf: equip
    UI_BoxEquipDrawBucketRelative(box, &drb);
    return interact;
}

engine_function UI_InteractResult
UI_HuePicker(F32 *hue, String8 string)
{
    UI_Box *box = UI_BoxMake(UI_BoxFlag_DrawBorder|UI_BoxFlag_Clickable, string);
    
    // rjf: interact
    UI_InteractResult interact = UI_BoxInteract(box);
    {
        if(interact.dragging)
        {
            F32 pct = (interact.mouse.x - box->rect.x0)/Dim2F32(box->rect).x;
            pct = Clamp(0, pct, 1);
            *hue = pct;
            
            UI_Tooltip
                UI_PrefWidth(UI_ChildrenSum(1))
                UI_PrefHeight(UI_ChildrenSum(1))
            {
                F32 padding = 10.f;
                UI_Box *container = UI_BoxMake(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, Str8Lit(""));
                UI_BoxEquipChildLayoutAxis(container, Axis2_Y);
                UI_PushParent(container);
                {
                    UI_Spacer(UI_Pixels(padding, 1.f));
                    UI_Row
                    {
                        UI_Spacer(UI_Pixels(padding, 1.f));
                        
                        // rjf: color preview
                        {
                            Vec3F32 rgb = RGBFromHSV(V3F32(*hue, 1, 1));
                            UI_PrefHeight(UI_Pixels(30.f, 1.f))
                                UI_BackgroundColor(V4F32(rgb.r, rgb.g, rgb.b, 1))
                                UI_Flags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder)
                                UI_CornerRadius(8.f)
                                UI_Spacer(UI_Pixels(30.f, 1.f));
                        }
                        
                        UI_Spacer(UI_Pixels(padding, 1.f));
                        
                        // rjf: breakdown by component
                        UI_PrefHeight(UI_ChildrenSum(1)) 
                            UI_Column
                            UI_PrefWidth(UI_TextDim(10, 1))
                            UI_PrefHeight(UI_TextDim(0, 1))
                        {
                            UI_LabelF("Hue: %.2f", *hue);
                        }
                        
                        UI_Spacer(UI_Pixels(padding, 1.f));
                    }
                    UI_Spacer(UI_Pixels(padding, 1.f));
                }
                UI_PopParent();
            }
        }
    }
    
    // rjf: rendering
    DR_Bucket drb = {0};
    {
        Rng2F32 rect = box->rect;
        F32 sliver_width = Dim2F32(rect).x / 6;
        F32 hues[] =
        {
            0.f / 6.f,
            1.f / 6.f,
            2.f / 6.f,
            3.f / 6.f,
            4.f / 6.f,
            5.f / 6.f,
            6.f / 6.f,
        };
        for(int idx = 0; idx < 6; idx += 1)
        {
            F32 hue0 = (F32)hues[idx];
            F32 hue1 = (F32)hues[idx+1];
            Vec3F32 color0 = RGBFromHSV(V3F32(hue0, 1, 1));
            Vec3F32 color1 = RGBFromHSV(V3F32(hue1, 1, 1));
            Rng2F32 sliver_rect =
            {
                rect.x0 + idx*sliver_width,
                rect.y0,
                rect.x0 + (idx+1)*sliver_width,
                rect.y1,
            };
            DR_Rect_VCB(&drb,
                        sliver_rect,
                        V4F32(color0.x, color0.y, color0.z, 1),
                        V4F32(color0.x, color0.y, color0.z, 1),
                        V4F32(color1.x, color1.y, color1.z, 1),
                        V4F32(color1.x, color1.y, color1.z, 1),
                        0, 0);
        }
        DR_Rect(&drb,
                R2F32(V2F32(rect.x0 + *hue * Dim2F32(rect).x - 1, rect.y0),
                      V2F32(rect.x0 + *hue * Dim2F32(rect).x + 1, rect.y1)),
                V4F32(1, 1, 1, 1));
    }
    
    // rjf: equip
    UI_BoxEquipDrawBucketRelative(box, &drb);
    return interact;
}
