////////////////////////////////
//~ rjf: Points + Ranges

engine_function TE_Point
TE_PointMake(S64 line, S64 column)
{
    TE_Point p = {0};
    p.line = line;
    p.column = column;
    return p;
}

engine_function B32
TE_PointMatch(TE_Point a, TE_Point b)
{
    return a.line == b.line && a.column == b.column;
}

engine_function B32
TE_PointLessThan(TE_Point a, TE_Point b)
{
    B32 result = 0;
    if(a.line < b.line)
    {
        result = 1;
    }
    else if(a.line == b.line)
    {
        result = a.column < b.column;
    }
    return result;
}

engine_function TE_Point
TE_MinPoint(TE_Point a, TE_Point b)
{
    TE_Point result = b;
    if(TE_PointLessThan(a, b))
    {
        result = a;
    }
    return result;
}

engine_function TE_Point
TE_MaxPoint(TE_Point a, TE_Point b)
{
    TE_Point result = a;
    if(TE_PointLessThan(a, b))
    {
        result = b;
    }
    return result;
}

engine_function TE_Range
TE_RangeMake(TE_Point min, TE_Point max)
{
    TE_Range range = {0};
    if(TE_PointLessThan(min, max))
    {
        range.min = min;
        range.max = max;
    }
    else
    {
        range.min = max;
        range.max = min;
    }
    return range;
}

engine_function B32
TE_CharIsScanBoundary(U8 c)
{
    return CharIsSpace(c) || c == '/' || c == '\\';
}

engine_function int
TE_ScanWord(String8 string, S64 start, B32 forward)
{
    int delta = 0;
    
    //- rjf: scan forward
    if(forward)
    {
        B32 found_space = 0;
        for(S64 s = start+1; s <= (S64)string.size+1; s += 1)
        {
            if(s < string.size)
            {
                if(found_space)
                {
                    if(!TE_CharIsScanBoundary(string.str[s-1]))
                    {
                        delta = (int)(s - start);
                        break;
                    }
                }
                else if(TE_CharIsScanBoundary(string.str[s-1]))
                {
                    found_space = 1;
                }
            }
            else
            {
                delta = ((int)string.size+1 - start);
            }
        }
    }
    //- rjf: scan backwards
    else
    {
        B32 found_non_space = 0;
        for(S64 s = start-1; s >= 1; s -= 1)
        {
            if(s == 1)
            {
                delta = -start + 1;
                break;
            }
            if(found_non_space)
            {
                if(TE_CharIsScanBoundary(string.str[s-1]))
                {
                    delta = (s+1)-start;
                    break;
                }
            }
            else if(!TE_CharIsScanBoundary(string.str[s-1]))
            {
                found_non_space = 1;
            }
        }
    }
    
    return delta;
}

engine_function void
TE_InputActionListPush(M_Arena *arena, TE_InputActionList *list, TE_InputAction action)
{
    TE_InputActionNode *n = PushArrayZero(arena, TE_InputActionNode, 1);
    n->action = action;
    QueuePush(list->first, list->last, n);
    list->count += 1;
}

engine_function String8
TE_ReplacedRangeStringFromString(M_Arena *arena, String8 string, Rng1S64 range, String8 replace)
{
    //- rjf: clamp range
    if(range.min > string.size)
    {
        range.min = 0;
    }
    if(range.max > string.size)
    {
        range.max = string.size;
    }
    
    //- rjf: calculate new size
    U64 old_size = string.size;
    U64 new_size = old_size - (range.max - range.min) + replace.size;
    
    //- rjf: push+fill new string storage
    U8 *push_base = PushArrayZero(arena, U8, new_size);
    {
        if(range.min > 0)
        {
            MemoryCopy(push_base+0, string.str, range.min);
        }
        if(range.max < string.size)
        {
            MemoryCopy(push_base+range.min+replace.size, string.str+range.max, string.size-range.max);
        }
        MemoryCopy(push_base+range.min, replace.str, replace.size);
    }
    
    return Str8(push_base, new_size);
}

engine_function TE_Op
TE_SingleLineOpFromInputAction(M_Arena *arena, TE_InputAction action, String8 line, TE_Point cursor, TE_Point mark)
{
    B32 good_action = (action.layer <= TE_InputActionLayer_SingleLine && action.codepoint != '\n');
    TE_Point next_cursor = cursor;
    TE_Point next_mark = mark;
    TE_Range range = TE_RangeMake(TE_PointMake(cursor.line, 1), TE_PointMake(cursor.line, 1));
    String8 line_past_whitespace = Str8SkipWhitespace(line);
    S64 first_non_ws_column = 1 + (U64)(line_past_whitespace.str - line.str);
    String8 replace = {0};
    B32 copy = 0;
    B32 paste = 0;
    B32 moved_line = 0;
    {
        if(good_action)
        {
            Vec2S32 delta = action.delta;
            Vec2S32 original_delta = delta;
            
            //- rjf: turn action's insertion codepoint into a replace string
            if(action.codepoint != 0 && action.codepoint != '\n')
            {
                U8 text[8] = {0};
                U32 advance = Utf8FromCodepoint(text, action.codepoint);
                replace = PushStr8Copy(arena, Str8(text, (U64)advance));
                range = TE_RangeMake(cursor, mark);
                next_cursor = range.min;
                next_cursor.column += 1;
                if(action.codepoint == '\n')
                {
                    good_action = 0;
                }
            }
            
            //- rjf: turn delta into word-wise delta
            if(action.flags & TE_InputActionFlag_WordScan)
            {
                delta.x = TE_ScanWord(line, cursor.column, delta.x > 0);
            }
            
            //- rjf: zero delta
            if(!TE_PointMatch(cursor, mark) && action.flags & TE_InputActionFlag_ZeroDeltaOnSelect)
            {
                delta = V2S32(0, 0);
            }
            
            //- rjf: form next cursor
            if(TE_PointMatch(cursor, mark) || !(action.flags & TE_InputActionFlag_ZeroDeltaOnSelect))
            {
                next_cursor.column += delta.x;
            }
            
            //- rjf: cap at line
            if(action.flags & TE_InputActionFlag_CapAtLine)
            {
                S64 min_column = first_non_ws_column;
                if(cursor.column == min_column)
                {
                    min_column = 1;
                }
                next_cursor.column = Clamp(min_column, next_cursor.column, (S64)(line.size+1));
            }
            
            //- rjf: in some cases, we want to pick a selection side based on the delta
            if(!TE_PointMatch(cursor, mark) && action.flags & TE_InputActionFlag_PickSelectSide)
            {
                if(original_delta.x < 0 || original_delta.y < 0)
                {
                    next_cursor = next_mark = TE_MinPoint(cursor, mark);
                    moved_line = 1;
                }
                else if(original_delta.x > 0 || original_delta.y > 0)
                {
                    next_cursor = next_mark = TE_MaxPoint(cursor, mark);
                    moved_line = 1;
                }
            }
            
            //- rjf: copying
            if(action.flags & TE_InputActionFlag_Copy)
            {
                copy = 1;
            }
            
            //- rjf: pasting
            if(action.flags & TE_InputActionFlag_Paste)
            {
                paste = 1;
                range = TE_RangeMake(cursor, mark);
            }
            
            //- rjf: deletion
            if(action.flags & TE_InputActionFlag_Delete)
            {
                TE_Point new_pos = TE_MinPoint(next_cursor, next_mark);
                range = TE_RangeMake(next_cursor, next_mark);
                replace = Str8Lit("");
                next_cursor = next_mark = new_pos;
            }
            
            //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
            if(!(action.flags & TE_InputActionFlag_KeepMark))
            {
                next_mark = next_cursor;
            }
        }
    }
    
    //- rjf: determine if this event should be taken, based on bounds/delta of cursor
    B32 taken = 0;
    {
        taken = good_action;
        if((!moved_line && next_cursor.column > line.size+replace.size+1 || next_cursor.column < 1) ||
           ((action.flags & TE_InputActionFlag_Delete) && TE_PointMatch(cursor, mark) && (range.min.column < 1 || line.size+1 < range.min.column)) ||
           ((action.flags & TE_InputActionFlag_Delete) && TE_PointMatch(cursor, mark) && (range.max.column < 1 || line.size+1 < range.max.column)))
        {
            taken = 0;
        }
        if(!moved_line)
        {
            next_cursor.column = Clamp(1, next_cursor.column, line.size+replace.size+1);
        }
    }
    
    //- rjf: build+fill result
    TE_Op op = {0};
    {
        op.taken   = taken;
        op.replace = replace;
        op.range   = range;
        op.cursor  = next_cursor;
        op.mark    = next_mark;
        op.copy    = copy;
        op.paste   = paste;
    }
    return op;
}

#if 0
engine_function TE_Op
TE_LocalMultiLineOpFromInputAction(M_Arena *arena, TE_InputAction action,
                                   String8 prev_line, TE_Range prev_line_rng,
                                   String8 line, TE_Range line_rng,
                                   String8 next_line, TE_Range next_line_rng,
                                   U64 line_count, TE_Point cursor, TE_Point mark,
                                   S64 preferred_column)
{
    TE_Op result = TE_LocalSingleLineOpFromInputAction(arena, action, line, cursor, mark);
    if(result.taken)
    {
        result.new_preferred_column = result.cursor.column;
    }
    else
    {
        B32 good_action = action.layer <= TE_InputActionLayer_LocalMultiLine;
        TE_Point next_cursor = cursor;
        TE_Point next_mark = mark;
        TE_Range range = TE_RangeMake(TE_PointMake(cursor.line, 1), TE_PointMake(cursor.line, 1));
        String8 replace = {0};
        
        if(good_action)
        {
            Vec2S32 delta = action.delta;
            
            //- rjf: turn action's insertion codepoint into a replace string
            if(action.codepoint != 0)
            {
                U8 text[8] = {0};
                U32 advance = Utf8FromCodepoint(text, action.codepoint);
                replace = PushStr8Copy(arena, Str8(text, (U64)advance));
                range = TE_RangeMake(cursor, mark);
                if(action.codepoint == '\n')
                {
                    next_cursor = next_mark = TE_PointMake(TE_MinPoint(cursor, mark).line+1, 1);
                }
                else
                {
                    next_cursor = next_mark = TE_PointMake(TE_MinPoint(cursor, mark).line,
                                                           TE_MinPoint(cursor, mark).column+1);
                }
            }
            
            //- rjf: wrap lines left
            if(delta.x < 0 && cursor.column <= 1 && next_cursor.line > 1)
            {
                next_cursor.column = prev_line.size+1;
                next_cursor.line -= 1;
                preferred_column = next_cursor.column;
            }
            
            //- rjf: wrap lines right
            if(delta.x > 0 && cursor.column == line.size+1 && next_cursor.line < line_count)
            {
                next_cursor.column = 1;
                next_cursor.line += 1;
                preferred_column = next_cursor.column;
            }
            
            //- rjf: vertical movement
            if(delta.y == +1 || delta.y == -1)
            {
                next_cursor.line += delta.y;
                next_cursor.line = Clamp(1, next_cursor.line, line_count);
                if(delta.y == -1)
                {
                    next_cursor.column = Clamp(1, preferred_column, prev_line.size+1);
                }
                if(delta.y == +1)
                {
                    next_cursor.column = Clamp(1, preferred_column, next_line.size+1);
                }
            }
            
            //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
            if(!(action.flags & TE_InputActionFlag_KeepMark))
            {
                next_mark = next_cursor;
            }
            
            //- rjf: form range across movement, if we are deleting
            if(action.flags & TE_InputActionFlag_Delete)
            {
                range = TE_RangeMake(next_cursor, cursor);
                next_cursor = next_mark = TE_MinPoint(next_cursor, range.min);
            }
        }
        TE_Op op = {0};
        {
            op.taken                = good_action;
            op.replace              = replace;
            op.range                = range;
            op.cursor               = next_cursor;
            op.mark                 = next_mark;
            op.new_preferred_column = preferred_column;
        }
        result = op;
    }
    return result;
}
#endif
