#include "engine_bundles/engine_full.h"
#include "editor.h"
#include "generated/editor.meta.c"

////////////////////////////////
//~ rjf: Globals

global ED_Global *ed_state = 0;

////////////////////////////////
//~ rjf: Cursor States

function void
ED_CursorStateSetCursor(ED_CursorState *cs, TE_Point point)
{
    // ED_Entity *entity = ED_EntityFromHandle(cs->cursor);
    // ED_EntityEquipTextPoint(entity, point);
}

function void
ED_CursorStateSetMark(ED_CursorState *cs, TE_Point point)
{
    // ED_Entity *entity = ED_EntityFromHandle(cs->mark);
    // ED_EntityEquipTextPoint(entity, point);
}

function TE_Point
ED_CursorFromCursorState(ED_CursorState *cs)
{
    TE_Point result = {0};
    // ED_Entity *entity = ED_EntityFromHandle(cs->cursor);
    // if(entity->flags & ED_EntityFlag_HasTextPoint)
    // {
    // result = entity->txt_point;
    // }
    return result;
}

function TE_Point
ED_MarkFromCursorState(ED_CursorState *cs)
{
    TE_Point result = {0};
    // ED_Entity *entity = ED_EntityFromHandle(cs->mark);
    // if(entity->flags & ED_EntityFlag_HasTextPoint)
    // {
    // result = entity->txt_point;
    // }
    return result;
}

function ED_CursorState *
ED_MainCursorStateFromView(ED_View *view)
{
    ED_CursorState *result = view->cursor_states.first;
    return result;
}

function ED_CursorState *
ED_ViewAllocCursorState(ED_View *view)
{
    ED_CursorState *cs = view->first_free_cursor_state;
    if(cs != 0)
    {
        view->first_free_cursor_state = view->first_free_cursor_state->next;
    }
    else
    {
        cs = PushArrayZero(view->arena, ED_CursorState, 1);
    }
    return cs;
}

////////////////////////////////
//~ rjf: Lexing

function ED_TokenKind
ED_TokenKindFromKey(String8 key)
{
    ED_TokenKind result = ED_TokenKind_Null;
    for(ED_TokenKind kind = (ED_TokenKind)(ED_TokenKind_Null+1);
        kind < ED_TokenKind_COUNT;
        kind = (ED_TokenKind)(kind + 1))
    {
        if(Str8Match(key, ed_g_token_kind_key_table[kind], MatchFlag_CaseInsensitive))
        {
            result = kind;
            break;
        }
    }
    return result;
}

function ED_Token
ED_TokenMake(ED_TokenKind kind, Rng1U64 range)
{
    ED_Token result = {0};
    result.kind = kind;
    result.range = range;
    return result;
}

function void
ED_TokenListPushNode(ED_TokenList *list, ED_TokenNode *node)
{
    QueuePush(list->first, list->last, node);
    list->count += 1;
}

function void
ED_TokenListPush(M_Arena *arena, ED_TokenList *list, ED_Token token)
{
    ED_TokenNode *node = PushArrayZero(arena, ED_TokenNode, 1);
    node->token = token;
    ED_TokenListPushNode(list, node);
}

function ED_TokenList
ED_TokenListFromString(M_Arena *arena, ED_LangRuleSet *lang_rules, String8 string, U64 start)
{
    ED_TokenList result = {0};
    if(lang_rules != 0)
    {
        //- rjf: build token list
        for(U64 idx = start; idx < string.size;)
        {
            U64 start_idx = idx;
            
            U64 best_token_size = 0;
            ED_TokenKind token_kind = ED_TokenKind_Null;
            for(ED_TokenKind kind = (ED_TokenKind)(ED_TokenKind_Null+1);
                kind < ED_TokenKind_COUNT;
                kind += 1)
            {
                B32 matches_this_kind = 0;
                U64 token_advance = SP_PatternMatchAdvanceFromString(Str8Skip(string, idx), lang_rules->token_patterns[kind]);
                if(token_advance != 0 && token_advance > best_token_size)
                {
                    best_token_size = token_advance;
                    matches_this_kind = 1;
                }
                
                B32 has_table = lang_rules->token_tables[kind].table_size != 0;
                B32 found_table_entry = 0;
                if(matches_this_kind && has_table)
                {
                    String8 token_string = Substr8(string, R1U64(start_idx, start_idx+token_advance));
                    U64 hash = ED_HashFromString(token_string);
                    U64 slot = hash % lang_rules->token_tables[kind].table_size;
                    for(ED_LangTokenStringNode *n = lang_rules->token_tables[kind].table[slot]; n != 0; n = n->next)
                    {
                        if(Str8Match(n->string, token_string, 0))
                        {
                            found_table_entry = 1;
                            break;
                        }
                    }
                }
                
                if(matches_this_kind && (found_table_entry || !has_table))
                {
                    token_kind = kind;
                }
                else if(matches_this_kind && !found_table_entry && has_table)
                {
                    best_token_size = 0;
                }
            }
            
            if(token_kind != ED_TokenKind_Null)
            {
                ED_TokenListPush(arena, &result, ED_TokenMake(token_kind, R1U64(start_idx, start_idx+best_token_size)));
                idx += best_token_size;
            }
            else
            {
                idx += 1;
            }
        }
    }
    return result;
}

function ED_Token
ED_TokenFromPointInList(U64 point, ED_TokenList list)
{
    ED_Token result = {0};
    for(ED_TokenNode *n = list.first; n != 0; n = n->next)
    {
        if(n->token.range.min <= point && point < n->token.range.max)
        {
            result = n->token;
            break;
        }
    }
    return result;
}

////////////////////////////////
//~ rjf: Token Patterns

function ED_TokenPatternPiece *
ED_TokenPatternPieceFromMDNode(M_Arena *arena, MD_Node *node)
{
    ED_TokenPatternPiece *result = 0;
    return result;
}

function ED_TokenPattern *
ED_TokenPatternFromMDNode(M_Arena *arena, MD_Node *node)
{
    ED_TokenPattern *pattern = 0;
    for(MD_EachNode(md_piece, node->first_child))
    {
        // rjf: single piece
        if(md_piece->string.size != 0)
        {
        }
        
        // rjf: several options
        else
        {
        }
    }
    return pattern;
}

function ED_TokenPatternMatchResult
ED_TokenPatternMatch(ED_TokenList tokens, ED_TokenPattern *pattern)
{
    ED_TokenPatternMatchResult result = {0};
    return result;
}

////////////////////////////////
//~ rjf: Token Ranges

function ED_TokenRangeNodeLoose *
ED_TokenRangeTreeFromPointArray_Loose(M_Arena *arena, TE_Point max_point, ED_TokenRangePointArray pts)
{
    ED_TokenRangeNodeLoose *root = PushArrayZero(arena, ED_TokenRangeNodeLoose, 1);
    root->range.kind = ED_TokenRangeKind_Null;
    root->range.range = TE_RangeMake(TE_PointMake(1, 1), max_point);
    ED_TokenRangeNodeLoose *active_parent = root;
    for(U64 idx = 0; idx < pts.count; idx += 1)
    {
        ED_TokenRangePoint p = pts.v[idx];
        B32 kind_forms_ranges = ed_g_token_range_point_kind_end_token_kind_table[p.kind] != ED_TokenKind_Null;
        if(p.starter)
        {
            ED_TokenRangeNodeLoose *n = PushArrayZero(arena, ED_TokenRangeNodeLoose, 1);
            n->parent = active_parent;
            n->range.kind = p.kind;
            n->range.range.min = p.point;
            if(active_parent)
            {
                DLLPushBack(active_parent->first, active_parent->last, n);
            }
            active_parent = n;
        }
        else
        {
            for(ED_TokenRangeNodeLoose *parent = active_parent; parent != 0; parent = parent->parent)
            {
                if(active_parent != root)
                {
                    active_parent = active_parent->parent;
                }
                parent->range.range.max = p.point;
                if(parent->range.kind == p.kind)
                {
                    break;
                }
            }
        }
    }
    for(ED_TokenRangeNodeLoose *child = root->first; child != 0; child = child->next)
    {
        if(child->range.range.max.line == 0 &&
           child->range.range.max.column == 0)
        {
            child->range.range.max = max_point;
        }
    }
    return root;
}

function ED_TokenRangeNodeLoose *
ED_EnclosingTokenRangeNodeFromPoint_Loose(ED_TokenRangeNodeLoose *root, TE_Point point)
{
    ED_TokenRangeNodeLoose *result = root;
    for(ED_TokenRangeNodeLoose *parent = root, *next = 0; parent != 0; parent = next)
    {
        next = 0;
        for(ED_TokenRangeNodeLoose *child = parent->first; child != 0; child = child->next)
        {
            if((TE_PointLessThan(child->range.range.min, point) || TE_PointMatch(child->range.range.min, point)) &&
               (TE_PointLessThan(point, child->range.range.max) || TE_PointMatch(point, child->range.range.max)))
            {
                next = result = child;
                break;
            }
        }
    }
    return result;
}

////////////////////////////////
//~ rjf: Language Rule Sets

function ED_LangRuleSet *
ED_LangRuleSetFromExtension(String8 extension)
{
    ED_LangRuleSet *result = 0;
    extension = Str8SkipLastPeriod(extension);
    
    // rjf: look up into the language table cache
    ED_LangNode *lang_node = 0;
    U64 hash = ED_HashFromString(extension);
    U64 slot = hash % ed_state->language_table_size;
    for(ED_LangNode *n = ed_state->language_table[slot]; n != 0; n = n->next)
    {
        if(Str8Match(n->extension, extension, MatchFlag_CaseInsensitive))
        {
            lang_node = n;
            break;
        }
    }
    
    // rjf: allocate if we didn't find one
    if(lang_node == 0)
    {
        M_Temp scratch = GetScratch(0, 0);
        String8 search_path = OS_GetSystemPath(scratch.arena, OS_SystemPath_Binary);
        search_path = PushStr8F(scratch.arena, "%S/", search_path);
        OS_FileIter *it = OS_FileIterBegin(scratch.arena, search_path);
        for(OS_FileInfo info = {0}; OS_FileIterNext(scratch.arena, it, &info) && lang_node == 0;)
        {
            String8 name = info.name;
            String8 lang_file_ext = Str8SkipLastPeriod(name);
            if(Str8Match(lang_file_ext, Str8Lit("tsl"), MatchFlag_CaseInsensitive))
            {
                String8 full_path = PushStr8F(scratch.arena, "%S%S", search_path, name);
                CFG_CachedFile *cfg_file = CFG_CachedFileFromPath(full_path);
                MD_Node *extensions = MD_ChildFromString(cfg_file->root, MD_S8Lit("extensions"), MD_StringMatchFlag_CaseInsensitive);
                B32 fits_this_extension = 0;
                for(MD_EachNode(ext_rule, extensions->first_child))
                {
                    if(Str8Match(Str8FromMD(ext_rule->string), extension, MatchFlag_CaseInsensitive))
                    {
                        fits_this_extension = 1;
                        break;
                    }
                }
                if(fits_this_extension)
                {
                    lang_node = PushArrayZero(ed_state->arena, ED_LangNode, 1);
                    lang_node->next = ed_state->language_table[slot];
                    ed_state->language_table[slot] = lang_node;
                    lang_node->extension = PushStr8Copy(ed_state->arena, extension);
                    lang_node->path = PushStr8Copy(ed_state->arena, full_path);
                }
            }
        }
        OS_FileIterEnd(it);
        ReleaseScratch(scratch);
    }
    
    // rjf: pull out root if we found one
    if(lang_node != 0)
    {
        CFG_CachedFile *cfg_file = CFG_CachedFileFromPath(lang_node->path);
        if(cfg_file->user_data == 0)
        {
            ED_LangRuleSet *rules = PushArrayZero(cfg_file->arena, ED_LangRuleSet, 1);
            lang_node->generation += 1;
            rules->generation = lang_node->generation;
            
            // rjf: build token patterns
            MD_Node *patterns_md_def = MD_ChildFromString(cfg_file->root, MD_S8Lit("token_patterns"), MD_StringMatchFlag_CaseInsensitive);
            for(ED_TokenKind kind = ED_TokenKind_Null;
                kind < ED_TokenKind_COUNT;
                kind = (ED_TokenKind)(kind + 1))
            {
                String8 key = ed_g_token_kind_key_table[kind];
                MD_Node *pattern_md_def = MD_ChildFromString(patterns_md_def, MDFromStr8(key), MD_StringMatchFlag_CaseInsensitive);
                rules->token_patterns[kind] = SP_PatternFromMDNode(cfg_file->arena, pattern_md_def);
            }
            
            // rjf: grab metadata about token range kinds
            MD_Node *ranges_md_def = MD_ChildFromString(cfg_file->root, MD_S8Lit("token_ranges"), MD_StringMatchFlag_CaseInsensitive);
            for(ED_TokenRangeKind kind = (ED_TokenRangeKind)(ED_TokenRangeKind_Null+1);
                kind < ED_TokenRangeKind_COUNT;
                kind = (ED_TokenRangeKind)(kind + 1))
            {
                MD_Node *rng_md_def = MD_ChildFromString(ranges_md_def, MDFromStr8(ed_g_token_range_point_kind_string_table[kind]), MD_StringMatchFlag_CaseInsensitive);
                MD_Node *nestable_def = MD_ChildFromString(rng_md_def, MD_S8Lit("nestable"), MD_StringMatchFlag_CaseInsensitive);
                B32 is_nestable = MD_S8Match(nestable_def->first_child->string, MD_S8Lit("1"), 0);
                rules->token_range_point_kind_is_nestable[kind] = is_nestable;
            }
            
            // rjf: build token tables
            MD_Node *tables_md_def = MD_ChildFromString(cfg_file->root, MD_S8Lit("token_tables"), MD_StringMatchFlag_CaseInsensitive);
            for(ED_TokenKind kind = ED_TokenKind_Null;
                kind < ED_TokenKind_COUNT;
                kind = (ED_TokenKind)(kind + 1))
            {
                String8 key = ed_g_token_kind_key_table[kind];
                MD_Node *table_md_def = MD_ChildFromString(tables_md_def, MDFromStr8(key), MD_StringMatchFlag_CaseInsensitive);
                U64 num_strings = MD_ChildCountFromNode(table_md_def);
                if(num_strings != 0)
                {
                    rules->token_tables[kind].table_size = UpToPow2_64(num_strings);
                    rules->token_tables[kind].table = PushArrayZero(cfg_file->arena, ED_LangTokenStringNode *, rules->token_tables[kind].table_size);
                    for(MD_EachNode(str, table_md_def->first_child))
                    {
                        ED_LangTokenStringNode *n = PushArrayZero(cfg_file->arena, ED_LangTokenStringNode, 1);
                        n->string = Str8FromMD(str->string);
                        U64 hash = ED_HashFromString(n->string);
                        U64 slot = hash%rules->token_tables[kind].table_size;
                        n->next = rules->token_tables[kind].table[slot];
                        rules->token_tables[kind].table[slot] = n;
                    }
                }
            }
            
            cfg_file->user_data = rules;
        }
        result = (ED_LangRuleSet *)cfg_file->user_data;
    }
    
    return result;
}

////////////////////////////////
//~ rjf: Buffers

//- rjf: basic type functions

function ED_Buffer *
ED_BufferNil(void)
{
    return &ed_state->nil_buffer;
}

function B32
ED_BufferIsNil(ED_Buffer *buffer)
{
    return buffer == 0 || buffer == ED_BufferNil();
}

function ED_Handle
ED_HandleFromBuffer(ED_Buffer *buffer)
{
    ED_Handle handle = {0};
    if(!ED_BufferIsNil(buffer))
    {
        handle.u64[0] = (U64)buffer;
        handle.u64[1] = buffer->alloc_generation;
    }
    return handle;
}

function ED_Buffer *
ED_BufferFromHandle(ED_Handle handle)
{
    ED_Buffer *buffer = (ED_Buffer *)handle.u64[0];
    if(ED_BufferIsNil(buffer) || buffer->alloc_generation != handle.u64[1] || !(buffer->flags & ED_BufferFlag_Ready))
    {
        buffer = ED_BufferNil();
    }
    return buffer;
}

//- rjf: buffer info accessors

function TE_Range
ED_TextRangeFromBuffer(ED_Buffer *buffer)
{
    S64 last_line_num = ED_LineCountFromBuffer(buffer);
    U64 last_line_size = ED_SizeFromBufferLineNum(buffer, last_line_num);
    TE_Range rng = TE_RangeMake(TE_PointMake(1, 1), TE_PointMake(last_line_num, last_line_size+1));
    return rng;
}

function U64
ED_LineCountFromBuffer(ED_Buffer *buffer)
{
    return buffer->line_count;
}

function String8
ED_StringFromBufferLineNum(M_Arena *arena, ED_Buffer *buffer, S64 line_num)
{
    String8 result = {0};
    if(1 <= line_num && line_num <= ED_LineCountFromBuffer(buffer))
    {
        result = PushStr8Copy(arena, buffer->lines[line_num-1].string);
    }
    return result;
}

function U64
ED_SizeFromBufferLineNum(ED_Buffer *buffer, S64 line_num)
{
    U64 size = 0;
    if(1 <= line_num && line_num <= ED_LineCountFromBuffer(buffer))
    {
        size = buffer->lines[line_num-1].string.size;
    }
    return size;
}

function Rng1U64
ED_ContributingTokenRangePointRangeFromBufferLineNum_GroundTruth(ED_Buffer *buffer, S64 line_num)
{
    B32 found_start = 0;
    U64 start = 0;
    U64 end = 0;
    for(U64 idx = 0; idx <= buffer->token_range_points.count; idx += 1)
    {
        if(idx == buffer->token_range_points.count)
        {
            if(!found_start)
            {
                start = end = idx;
            }
            break;
        }
        else if(buffer->token_range_points.v[idx].point.line == line_num)
        {
            if(!found_start)
            {
                found_start = 1;
                start = idx;
            }
            if(found_start)
            {
                end = idx+1;
            }
        }
        else if(found_start)
        {
            break;
        }
        else if(line_num < buffer->token_range_points.v[idx].point.line)
        {
            start = idx;
            end = idx;
            break;
        }
    }
    Rng1U64 result = R1U64(start, end);
    return result;
}

function ED_Line *
ED_BufferLineFromNum(ED_Buffer *buffer, S64 line_num)
{
    ED_Line *result = 0;
    if(1 <= line_num && line_num <= ED_LineCountFromBuffer(buffer))
    {
        result = buffer->lines + line_num - 1;
    }
    return result;
}

//- rjf: tokens & token ranges

function ED_TokenList
ED_TokenListFromBufferLineNum(ED_LangRuleSet *lang_rules, ED_Buffer *buffer, S64 line_num)
{
    ED_TokenList result = {0};
    if(1 <= line_num && line_num <= ED_LineCountFromBuffer(buffer))
    {
        ED_Line *line = &buffer->lines[line_num-1];
        
        // rjf: re-lex line if its cache key is stale
        if(lang_rules != 0 &&
           (line->generation != line->cached_tokens_generation ||
            line->cached_tokens_rule_generation != lang_rules->generation ||
            line_num >= buffer->first_invalid_line_num) &&
           buffer->token_cache_arena != 0)
        {
            line->cached_tokens_generation = line->generation;
            line->cached_tokens_rule_generation = lang_rules->generation;
            M_Temp scratch = GetScratch(0, 0);
            ED_TokenRangeNodeLoose *rng_root = ED_TokenRangeTreeFromBuffer_Loose(buffer);
            String8 string = ED_StringFromBufferLineNum(scratch.arena, buffer, line_num);
            
            // rjf: get contributing token range point array
            Rng1U64 tokenrng_pt_array_contributed_range = ED_ContributingTokenRangePointRangeFromBufferLineNum_GroundTruth(buffer, line_num);
            U64 tokenrng_pt_array_insert_idx = tokenrng_pt_array_contributed_range.min;
            
            // rjf: free existing tokens
            for(ED_TokenNode *n = line->cached_tokens.first, *next = 0; n != 0; n = next)
            {
                next = n->next;
                n->next = buffer->first_free_token_node;
                buffer->first_free_token_node = n;
            }
            
            // rjf: zero cache list
            MemoryZeroStruct(&line->cached_tokens);
            
            // rjf: reconstruct tokens
            ED_TokenList tokens = ED_TokenListFromString(scratch.arena, lang_rules, string, 0);
            
            // rjf: look for any token range starters/enders
            ED_TokenList range_endpoints = {0};
            {
                for(ED_TokenNode *n = tokens.first; n != 0; n = n->next)
                {
                    B32 is_endpoint = ed_g_token_kind_is_range_endpoint_table[n->token.kind];
                    if(is_endpoint)
                    {
                        ED_TokenListPush(scratch.arena, &range_endpoints, n->token);
                    }
                }
            }
            
            // rjf: grow token range array if needed
            U64 token_range_points_to_add = range_endpoints.count;
            U64 token_range_points_to_remove = tokenrng_pt_array_contributed_range.max - tokenrng_pt_array_contributed_range.min;
            S64 token_range_point_count_delta = (token_range_points_to_add - token_range_points_to_remove);
            if(buffer->token_range_points.count + token_range_point_count_delta > buffer->token_range_point_cap)
            {
                U64 new_cap = UpToPow2_64(buffer->token_range_point_cap + token_range_point_count_delta);
                U64 cap_delta = new_cap - buffer->token_range_point_cap;
                ED_TokenRangePoint *push_base = PushArrayZero(buffer->token_range_point_arena, ED_TokenRangePoint, cap_delta);
                if(buffer->token_range_points.v == 0)
                {
                    buffer->token_range_points.v = push_base;
                }
                buffer->token_range_point_cap = new_cap;
            }
            
            // rjf: remove old token range points + add new ones
            {
                if(token_range_point_count_delta != 0 && tokenrng_pt_array_contributed_range.max < buffer->token_range_points.count)
                {
                    MemoryMove(buffer->token_range_points.v + tokenrng_pt_array_contributed_range.max + token_range_point_count_delta,
                               buffer->token_range_points.v + tokenrng_pt_array_contributed_range.max,
                               sizeof(ED_TokenRangePoint) * (buffer->token_range_points.count - tokenrng_pt_array_contributed_range.max));
                }
                U64 write_idx = 0;
                for(ED_TokenNode *n = range_endpoints.first; n != 0; n = n->next, write_idx += 1)
                {
                    ED_TokenRangeKind kind = ED_TokenRangeKind_Null;
                    B32 starter = 0;
                    {
                        for(ED_TokenRangeKind rng_kind = ED_TokenRangeKind_Null;
                            rng_kind < ED_TokenRangeKind_COUNT;
                            rng_kind += 1)
                        {
                            if(n->token.kind == ed_g_token_range_point_kind_start_token_kind_table[rng_kind])
                            {
                                starter = 1;
                                kind = rng_kind;
                                break;
                            }
                            else if(n->token.kind == ed_g_token_range_point_kind_end_token_kind_table[rng_kind])
                            {
                                starter = 0;
                                kind = rng_kind;
                                break;
                            }
                        }
                    }
                    
                    ED_TokenRangePoint *pt = buffer->token_range_points.v + tokenrng_pt_array_insert_idx + write_idx;
                    buffer->token_range_point_id_gen += 1;
                    pt->kind = kind;
                    pt->point = TE_PointMake(line_num, n->token.range.min+1 + (!starter) * (n->token.range.max-n->token.range.min));
                    pt->id = buffer->token_range_point_id_gen;
                    pt->starter = starter;
                }
            }
            
            // rjf: apply token range point delta
            if(token_range_point_count_delta != 0)
            {
                buffer->token_range_points.count += token_range_point_count_delta;
            }
            
            // rjf: post-process tokens to new list, given enclosing ranges
            ED_TokenList post_processed_tokens = {0};
            for(ED_TokenNode *n = tokens.first; n != 0; n = n->next)
            {
                ED_Token token = n->token;
                TE_Point start_of_token = TE_PointMake(line_num, token.range.min+1);
                ED_TokenRangeNodeLoose *enclosing_range_node = ED_EnclosingTokenRangeNodeFromPoint_Loose(rng_root, start_of_token);
                ED_TokenRange overriding_range = {0};
                for(ED_TokenRangeNodeLoose *r = enclosing_range_node; r != 0; r = r->parent)
                {
                    if(ed_g_token_range_point_kind_content_token_kind_table[r->range.kind] != ED_TokenKind_Null)
                    {
                        overriding_range = r->range;
                    }
                }
                ED_TokenKind rng_content_token_kind = ed_g_token_range_point_kind_content_token_kind_table[overriding_range.kind];
                
                if(token.kind != ed_g_token_range_point_kind_end_token_kind_table[overriding_range.kind] &&
                   rng_content_token_kind != ED_TokenKind_Null)
                {
                    token.kind = rng_content_token_kind;
                }
                ED_TokenListPush(scratch.arena, &post_processed_tokens, token);
            }
            
            // rjf: store tokens
            for(ED_TokenNode *n = post_processed_tokens.first; n != 0; n = n->next)
            {
                ED_TokenNode *store = buffer->first_free_token_node;
                if(store == 0)
                {
                    store = PushArrayZero(buffer->token_cache_arena, ED_TokenNode, 1);
                }
                else
                {
                    buffer->first_free_token_node = buffer->first_free_token_node->next;
                }
                store->token = n->token;
                ED_TokenListPushNode(&line->cached_tokens, store);
            }
            
            // rjf: if this line is the first invalid, we can bump
            if(line_num == buffer->first_invalid_line_num)
            {
                buffer->first_invalid_line_num += 1;
            }
            
            // rjf: if this line made changes to the token range points, we
            // need to invalidate all later lines, and rebuild the buffer's
            // cached range tree
            if(token_range_point_count_delta != 0 || (tokenrng_pt_array_contributed_range.max != tokenrng_pt_array_contributed_range.min))
            {
                buffer->first_invalid_line_num = line_num+1;
                if(buffer->token_range_tree_arena)
                {
                    TE_Range whole_range = ED_TextRangeFromBuffer(buffer);
                    M_ArenaClear(buffer->token_range_tree_arena);
                    buffer->token_range_tree_root = ED_TokenRangeTreeFromPointArray_Loose(buffer->token_range_tree_arena,
                                                                                          whole_range.max,
                                                                                          buffer->token_range_points);
                }
            }
            
            ReleaseScratch(scratch);
        }
        result = line->cached_tokens;
    }
    return result;
}

function ED_TokenRangeNodeLoose *
ED_TokenRangeTreeFromBuffer_Loose(ED_Buffer *buffer)
{
    return buffer->token_range_tree_root;
}

//- rjf: text mutation and baking

function void
ED_BufferEquipLineEndingEncoding(ED_Buffer *buffer, ED_LineEndingEncoding encoding)
{
    buffer->line_ending_encoding = encoding;
}

function void
ED_BufferLineReplaceRange(ED_LangRuleSet *lang_rules, ED_Buffer *buffer, ED_Line *line, Rng1S64 range, String8 string)
{
    M_Temp scratch = GetScratch(0, 0);
    S64 line_num = (line - buffer->lines) + 1;
    
    // rjf: clamp range
    Rng1S64 line_range = {1, line->string.size+1};
    if(range.min < line_range.min)
    {
        range.min = line_range.min;
    }
    if(line_range.max < range.max)
    {
        range.max = line_range.max;
    }
    
    // rjf: form new string, post-replace
    String8 new_string = TE_ReplacedRangeStringFromString(scratch.arena, line->string, R1S64(range.min-1, range.max-1), string);
    
    // rjf: commit new string to storage
    {
        if(new_string.size == 0)
        {
            M_HeapReleaseData(buffer->txt_heap, line->string.str);
            line->cap = 0;
            line->string.str = 0;
        }
        else if(new_string.size > line->cap)
        {
            M_HeapReleaseData(buffer->txt_heap, line->string.str);
            line->cap = UpToPow2_64(new_string.size);
            line->string.str = M_HeapAllocData(buffer->txt_heap, line->cap);
        }
        else if(line->cap > UpToPow2_64(new_string.size)<<2)
        {
            M_HeapReleaseData(buffer->txt_heap, line->string.str);
            line->cap = UpToPow2_64(new_string.size);
            line->cap = ClampBot(line->cap, 16);
            line->string.str = M_HeapAllocData(buffer->txt_heap, line->cap);
        }
        line->string.size = new_string.size;
        if(new_string.size != 0)
        {
            MemoryCopy(line->string.str, new_string.str, new_string.size);
        }
    }
    
    // rjf: if there was a substantial change, re-lex
    if(string.size != 0 || range.max != range.min)
    {
        line->generation += 1;
        ED_TokenListFromBufferLineNum(lang_rules, buffer, line_num);
    }
    
    ReleaseScratch(scratch);
}

function TE_Range
ED_BufferReplaceRange(ED_LangRuleSet *lang_rules, ED_Buffer *buffer, TE_Range range, String8 string)
{
    M_Temp scratch = GetScratch(0, 0);
    String8 last_line_string = ED_StringFromBufferLineNum(scratch.arena, buffer, ED_LineCountFromBuffer(buffer));
    TE_Range file_range = TE_RangeMake(TE_PointMake(1, 1),
                                       TE_PointMake(ED_LineCountFromBuffer(buffer), last_line_string.size+1));
    if(TE_PointLessThan(range.min, file_range.min))
    {
        range.min = file_range.min;
    }
    if(TE_PointLessThan(range.max, file_range.min))
    {
        range.max = file_range.min;
    }
    if(TE_PointLessThan(file_range.max, range.min))
    {
        range.min = file_range.max;
    }
    if(TE_PointLessThan(file_range.max, range.max))
    {
        range.max = file_range.max;
    }
    
    //- rjf: calculate line count delta
    S64 line_delta = 0;
    U64 lines_to_add = 0;
    U64 lines_to_remove = 0;
    {
        for(U64 idx = 0; idx < string.size; idx += 1)
        {
            if(string.str[idx] == '\n')
            {
                lines_to_add += 1;
            }
        }
        lines_to_remove = range.max.line - range.min.line;
        line_delta = (S64)lines_to_add - (S64)lines_to_remove;
    }
    
    //- rjf: expand lines storage if needed
    U64 old_line_count = buffer->line_count;
    U64 new_line_count = buffer->line_count + line_delta;
    {
        if(line_delta > 0)
        {
            if(new_line_count > buffer->line_cap)
            {
                U64 new_cap = UpToPow2_64(new_line_count);
                PushArrayZero(buffer->line_arena, ED_Line, new_cap-buffer->line_cap);
                buffer->line_cap = new_cap;
            }
        }
    }
    
    //- rjf: gather kept parts from first/last line
    ED_Line *first_line = buffer->lines + (range.min.line-1);
    ED_Line *last_line  = buffer->lines + (range.max.line-1);
    String8 first_line_pre_selection = PushStr8Copy(scratch.arena, Prefix8(first_line->string, range.min.column-1));
    String8 last_line_post_selection = PushStr8Copy(scratch.arena, Str8Skip(last_line->string, range.max.column-1));
    
    //- rjf: shift unaffected lines & free lines we're getting rid of
    if(line_delta != 0)
    {
        for(S64 line_num = range.min.line + 1;
            line_num <= range.max.line;
            line_num += 1)
        {
            ED_Line *line = buffer->lines + line_num-1;
            ED_BufferLineReplaceRange(lang_rules, buffer, line, R1S64(1, line->string.size+1), Str8Lit(""));
        }
        if(buffer->line_count > range.max.line)
        {
            MemoryMove(buffer->lines + (range.min.line) + lines_to_add,
                       buffer->lines + (range.max.line),
                       sizeof(ED_Line) * (buffer->line_count - range.max.line));
        }
        if(lines_to_add != 0)
        {
            MemoryZero(buffer->lines + (range.min.line), sizeof(ED_Line) * lines_to_add);
        }
        
        // rjf: bump range points
        {
            B32 bumped = 0;
            Rng1U64 cont_rng = ED_ContributingTokenRangePointRangeFromBufferLineNum_GroundTruth(buffer, range.min.line);
            for(U64 idx = cont_rng.min; idx < buffer->token_range_points.count; idx += 1)
            {
                bumped = 1;
                if(TE_PointLessThan(range.min, buffer->token_range_points.v[idx].point))
                {
                    buffer->token_range_points.v[idx].point.line += line_delta;
                }
            }
            
            // rjf: re-build token range tree
            if(bumped != 0)
            {
                if(buffer->token_range_tree_arena)
                {
                    TE_Range whole_range = ED_TextRangeFromBuffer(buffer);
                    M_ArenaClear(buffer->token_range_tree_arena);
                    buffer->token_range_tree_root = ED_TokenRangeTreeFromPointArray_Loose(buffer->token_range_tree_arena,
                                                                                          whole_range.max,
                                                                                          buffer->token_range_points);
                }
            }
        }
    }
    
    //- rjf: cut start line, if we inserted a newline
    if(lines_to_add != 0)
    {
        ED_BufferLineReplaceRange(lang_rules, buffer, first_line, R1S64(range.min.column, first_line->string.size+1), Str8Lit(""));
    }
    
    //- rjf: apply line delta
    buffer->line_count = new_line_count;
    
    //- rjf: apply ops
    TE_Point min_point = {0};
    TE_Point max_point = {0};
    {
        U64 start_idx = 0;
        S64 line_num = range.min.line;
        for(U64 idx = 0; idx <= string.size; idx += 1)
        {
            B32 newline = (string.size > 0 ? (string.str[idx] == '\n') : 0);
            if(idx == string.size || newline)
            {
                ED_Line *line = buffer->lines + line_num-1;
                String8 line_insert = Substr8(string, R1U64(start_idx, idx));
                line_insert = Str8SkipChopNewlines(line_insert);
                start_idx = idx+1;
                Rng1S64 range_in_line = R1S64(1, line->string.size+1);
                if(line_num == range.min.line)
                {
                    range_in_line.min = range.min.column;
                    min_point = max_point = TE_PointMake(line_num, range_in_line.min);
                }
                if(line_num == range.max.line)
                {
                    range_in_line.max = range.max.column;
                }
                if(line_num == range.min.line + line_delta)
                {
                    max_point = TE_PointMake(line_num, range_in_line.min+line_insert.size);
                }
                ED_BufferLineReplaceRange(lang_rules, buffer, line, range_in_line, line_insert);
            }
            if(newline)
            {
                line_num += 1;
            }
        }
    }
    
    //- rjf: stitch end line, if we inserted a newline
    if(line_delta != 0 && last_line_post_selection.size != 0)
    {
        ED_Line *line = buffer->lines + range.min.line-1 + lines_to_add;
        ED_BufferLineReplaceRange(lang_rules, buffer, line, R1S64(line->string.size+1, line->string.size+1), last_line_post_selection);
    }
    
    //- rjf: bump edit counter on nontrivial edits
    if(!TE_PointMatch(range.min, range.max) || string.size != 0)
    {
        buffer->edit_index += 1;
    }
    
    //- rjf: get result (range of inserted text)
    TE_Range result = TE_RangeMake(min_point, max_point);
    
    //- rjf: return
    ReleaseScratch(scratch);
    return result;
}

function String8List
ED_BakedStringListFromBufferRange(M_Arena *arena, ED_Buffer *buffer, TE_Range range)
{
    String8List result = {0};
    for(S64 line_num = range.min.line; line_num <= range.max.line; line_num += 1)
    {
        String8 string = ED_StringFromBufferLineNum(arena, buffer, line_num);
        if(line_num == range.min.line)
        {
            string = Str8Skip(string, range.min.column-1);
        }
        if(line_num == range.max.line)
        {
            string = Prefix8(string, range.max.column-range.min.column);
        }
        Str8ListPush(arena, &result, string);
        if(line_num < range.max.line)
        {
            switch(buffer->line_ending_encoding)
            {
                default: break;
                case ED_LineEndingEncoding_LF:
                {
                    Str8ListPush(arena, &result, Str8Lit("\n"));
                }break;
                case ED_LineEndingEncoding_CRLF:
                {
                    Str8ListPush(arena, &result, Str8Lit("\r\n"));
                }break;
            }
        }
    }
    return result;
}

function String8List
ED_BakedStringListFromBuffer(M_Arena *arena, ED_Buffer *buffer)
{
    return ED_BakedStringListFromBufferRange(arena, buffer, ED_TextRangeFromBuffer(buffer));
}

//- rjf: searching

function TE_Point
ED_BufferFindNeedle(ED_Buffer *buffer, TE_Point start, B32 forward, String8 needle, MatchFlags flags)
{
    TE_Point next = {0};
    if(!forward)
    {
        flags |= MatchFlag_FindLast;
    }
    for(S64 line_num = start.line;
        1 <= line_num && line_num <= ED_LineCountFromBuffer(buffer);
        line_num = (forward ? (line_num+1) : (line_num-1)))
    {
        M_Temp scratch = GetScratch(0, 0);
        String8 whole_line_string = ED_StringFromBufferLineNum(scratch.arena, buffer, line_num);
        S64 start_column = 1;
        S64 end_column = whole_line_string.size+1;
        if(line_num == start.line && forward)
        {
            start_column = start.column+1;
        }
        else if(line_num == start.line && !forward)
        {
            end_column = start.column;
        }
        String8 search_string = Substr8(whole_line_string, R1U64(start_column-1, end_column-1));
        U64 pos_of_needle = FindSubstr8(search_string, needle, 0, flags);
        ReleaseScratch(scratch);
        if(pos_of_needle < search_string.size)
        {
            next = TE_PointMake(line_num, start_column + pos_of_needle);
            break;
        }
    }
    return next;
}

//- rjf: auto-indentation

function void
ED_BufferAutoIndentLineNumRange(ED_LangRuleSet *lang_rules, ED_Buffer *buffer, Rng1S64 rng)
{
    M_Temp scratch = GetScratch(0, 0);
    U8 auto_indent_char = CFG_B32FromKey(Str8Lit("auto_indent_with_tabs")) ? '\t' : ' ';
    S64 auto_indent_space = CFG_S64FromKey(Str8Lit("auto_indent_size"));
    auto_indent_space = Clamp(0, auto_indent_space, 16);
    U64 line_count = ED_LineCountFromBuffer(buffer);
    ED_TokenRangeNodeLoose *rng_root = ED_TokenRangeTreeFromBuffer_Loose(buffer);
    for(S64 line_num = rng.min; line_num <= rng.max; line_num += 1)
    {
        if(0 < line_num && line_num <= line_count)
        {
            ED_TokenRangeNodeLoose *range = ED_EnclosingTokenRangeNodeFromPoint_Loose(rng_root, TE_PointMake(line_num, 1));
            S64 indent = 0;
            for(ED_TokenRangeNodeLoose *n = range; n != 0; n = n->parent)
            {
                if(n->range.kind == ED_TokenRangeKind_Scope)
                {
                    indent += 1;
                }
            }
            if(line_num == range->range.range.min.line || line_num == range->range.range.max.line)
            {
                indent -= 1;
            }
            indent = ClampBot(0, indent);
            
            String8 line_string = ED_StringFromBufferLineNum(scratch.arena, buffer, line_num);
            String8 line_string_post_ws = Str8SkipWhitespace(line_string);
            U64 num_leading_ws = line_string.size - line_string_post_ws.size;
            TE_Range indent_range = TE_RangeMake(TE_PointMake(line_num, 1),
                                                 TE_PointMake(line_num, 1+num_leading_ws));
            String8 new_indent_str = PushStr8FillByte(scratch.arena, indent*auto_indent_space, auto_indent_char);
            ED_BufferReplaceRange(lang_rules, buffer, indent_range, new_indent_str);
        }
    }
    ReleaseScratch(scratch);
}

//- rjf: main allocation/releasing

function ED_Buffer *
ED_BufferAlloc(ED_LangRuleSet *lang_rules, String8 text)
{
    ED_Buffer *buffer = ed_state->free_buffer;
    if(!ED_BufferIsNil(buffer))
    {
        ed_state->free_buffer = ed_state->free_buffer->next;
        U64 generation = buffer->alloc_generation;
        MemoryZeroStruct(buffer);
        buffer->alloc_generation = generation;
    }
    else
    {
        buffer = PushArrayZero(ed_state->arena, ED_Buffer, 1);
    }
    buffer->alloc_generation += 1;
    buffer->line_arena = M_ArenaAlloc(Gigabytes(16));
    buffer->txt_heap = M_HeapAlloc(Gigabytes(64));
    buffer->line_count = 1;
    buffer->line_cap = 256;
    buffer->lines = PushArrayZero(buffer->line_arena, ED_Line, buffer->line_cap);
    ED_BufferReplaceRange(0, buffer, TE_RangeMake(TE_PointMake(1, 1), TE_PointMake(1, 1)), text);
    buffer->token_cache_arena = M_ArenaAlloc(Gigabytes(16));
    buffer->token_range_point_arena = M_ArenaAlloc(Gigabytes(16));
    buffer->token_range_tree_arena = M_ArenaAlloc(Gigabytes(8));
    for(ED_ChangeDirection dir = (ED_ChangeDirection)0;
        dir < ED_ChangeDirection_COUNT;
        dir = (ED_ChangeDirection)(dir + 1))
    {
        buffer->change_stack_arenas[dir] = M_ArenaAlloc(Gigabytes(16));
    }
    buffer->first_invalid_line_num = 1;
    
    // rjf: figure out line-ending encoding
    ED_LineEndingEncoding le_encoding = ED_LineEndingEncoding_Null;
    {
        U64 crlf_counter = 0;
        U64 lf_counter = 0;
        for(U64 idx = 0; idx < text.size; idx += 1)
        {
            if(text.str[idx] == '\r' && text.str[idx+1] == '\n')
            {
                crlf_counter += 1;
                idx += 1;
            }
            else if(text.str[idx] == '\n')
            {
                lf_counter += 1;
            }
            if((lf_counter > 0 || crlf_counter > 0) && idx > 4096)
            {
                break;
            }
        }
        le_encoding = ((crlf_counter > 0 && crlf_counter > lf_counter)
                       ? ED_LineEndingEncoding_CRLF
                       : lf_counter > 0
                       ? ED_LineEndingEncoding_LF
                       : ED_LineEndingEncoding_LF);
    }
    buffer->line_ending_encoding = le_encoding;
    
    return buffer;
}

function void
ED_BufferRelease(ED_Buffer *buffer)
{
    if(!ED_BufferIsNil(buffer))
    {
        if(buffer->line_arena != 0)
        {
            M_ArenaRelease(buffer->line_arena);
        }
        if(buffer->token_cache_arena != 0)
        {
            M_ArenaRelease(buffer->token_cache_arena);
        }
        if(buffer->token_range_point_arena != 0)
        {
            M_ArenaRelease(buffer->token_range_point_arena);
        }
        for(ED_ChangeDirection dir = (ED_ChangeDirection)0;
            dir < ED_ChangeDirection_COUNT;
            dir = (ED_ChangeDirection)(dir + 1))
        {
            if(buffer->change_stack_arenas[dir])
            {
                M_ArenaRelease(buffer->change_stack_arenas[dir]);
            }
        }
        if(buffer->txt_heap != 0)
        {
            M_HeapRelease(buffer->txt_heap);
        }
        
        buffer->next = ed_state->free_buffer;
        ed_state->free_buffer = buffer;
        buffer->alloc_generation += 1;
    }
}

////////////////////////////////
//~ rjf: Entity With buffers

function ED_LangRuleSet *
ED_LangRuleSetFromEntity(C_Entity *entity)
{
    M_Temp scratch = GetScratch(0, 0);
    String8 entity_path = C_FullPathFromEntity(scratch.arena, entity);
    String8 extension = Str8SkipLastPeriod(entity_path);
    ED_LangRuleSet *lang_rules = ED_LangRuleSetFromExtension(extension);
    ReleaseScratch(scratch);
    return lang_rules;
}

////////////////////////////////
//~ rjf: Undo/Redo Operations

function M_Arena *
ED_ChangeStackArenaFromBufferDirection(ED_Buffer *buffer, ED_ChangeDirection direction)
{
    return buffer->change_stack_arenas[direction];
}

function ED_ChangeRecord *
ED_ChangeStackTopFromBufferDirection(ED_Buffer *buffer, ED_ChangeDirection direction)
{
    return buffer->change_stack_top[direction];
}

function ED_ChangeRecord *
ED_PushChangeRecord(ED_Buffer *buffer, ED_ChangeDirection direction, U64 glue_code)
{
    M_Arena *arena = buffer->change_stack_arenas[direction];
    U64 arena_pos = M_ArenaGetPos(arena);
    ED_ChangeRecord *record = PushArrayZero(arena, ED_ChangeRecord, 1);
    StackPush(buffer->change_stack_top[direction], record);
    record->arena_pos = arena_pos;
    record->glue_code = glue_code;
    return record;
}

function void
ED_PushTextOp(M_Arena *arena, ED_ChangeRecord *record, TE_Range range, String8 replaced)
{
    ED_TextOp *op = PushArrayZero(arena, ED_TextOp, 1);
    op->replaced = PushStr8Copy(arena, replaced);
    op->range = range;
    QueuePushFront(record->first_text_op, record->last_text_op, op);
}

function void
ED_PushCursorStateOp(M_Arena *arena, ED_ChangeRecord *record, TE_Point cursor, TE_Point mark)
{
    ED_CursorStateOp *op = PushArrayZero(arena, ED_CursorStateOp, 1);
    op->cursor = cursor;
    op->mark = mark;
    QueuePush(record->first_cs_op, record->last_cs_op, op);
}

function void
ED_ApplyNextChangeRecord(ED_View *view, ED_LangRuleSet *lang_rules, ED_Buffer *buffer, ED_ChangeDirection direction)
{
    M_Temp scratch = GetScratch(0, 0);
    ED_ChangeRecord *record = buffer->change_stack_top[direction];
    if(record != 0)
    {
        StackPop(buffer->change_stack_top[direction]);
        
        // rjf: push opposite record to to opposite direction
        ED_ChangeDirection reverse_dir = ED_ChangeDirectionFlip(direction);
        M_Arena *reverse_arena = ED_ChangeStackArenaFromBufferDirection(buffer, reverse_dir);
        ED_ChangeRecord *reverse_record = ED_PushChangeRecord(buffer, reverse_dir, 0);
        
        // rjf: push reverse-direction cursor ops
        for(ED_CursorState *cs = view->cursor_states.first; cs != 0; cs = cs->next)
        {
            TE_Point cursor = ED_CursorFromCursorState(cs);
            TE_Point mark = ED_MarkFromCursorState(cs);
            ED_PushCursorStateOp(reverse_arena, reverse_record, cursor, mark);
        }
        
        // rjf: apply text ops
        for(ED_TextOp *op = record->first_text_op; op != 0; op = op->next)
        {
            String8List reverse_replaced_strs = ED_BakedStringListFromBufferRange(scratch.arena, buffer, op->range);
            String8 reverse_replaced = Str8ListJoin(scratch.arena, reverse_replaced_strs, 0);
            TE_Range reverse_range = ED_BufferReplaceRange(lang_rules, buffer, op->range, op->replaced);
            ED_PushTextOp(reverse_arena, reverse_record, reverse_range, reverse_replaced);
        }
        
        // rjf: apply cursor ops
        if(record->first_cs_op != 0)
        {
            ED_ViewCloseExtraCursorMarkPairs(view);
            for(ED_CursorStateOp *op = record->first_cs_op; op != 0; op = op->next)
            {
                ED_CursorState *cs = ED_MainCursorStateFromView(view);
                if(op != record->first_cs_op)
                {
                    cs = ED_ViewAllocCursorState(view);
                }
                ED_CursorStateSetCursor(cs, op->cursor);
                ED_CursorStateSetMark(cs, op->mark);
            }
        }
        
        // rjf: pop the arena
        M_ArenaSetPosBack(buffer->change_stack_arenas[direction], record->arena_pos);
    }
    ReleaseScratch(scratch);
}

function void
ED_ClearChangeRecordStack(ED_Buffer *buffer, ED_ChangeDirection direction)
{
    M_ArenaClear(buffer->change_stack_arenas[direction]);
    buffer->change_stack_top[direction] = 0;
}

////////////////////////////////
//~ rjf: Text Layout Caches

function ED_TextLayoutCache *
ED_TextLayoutCacheAlloc(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(8));
    ED_TextLayoutCache *result = PushArrayZero(arena, ED_TextLayoutCache, 1);
    result->arena = arena;
    result->line_layout_arena = M_ArenaAlloc(Gigabytes(8));
    result->line_layout_count = 0;
    result->line_layout_cap = 0;
    return result;
}

function void
ED_TextLayoutCacheRelease(ED_TextLayoutCache *cache)
{
    M_ArenaRelease(cache->line_layout_arena);
    M_ArenaRelease(cache->arena);
}

function void
ED_TextLayoutCacheUpdate(ED_TextLayoutCache *cache, ED_Buffer *buffer, R_Font font, Rng1F32 view_range, F32 width_constraint)
{
    M_Temp scratch = GetScratch(0, 0);
    cache->line_height = font.line_advance;
    cache->line_count = ED_LineCountFromBuffer(buffer);
    Rng1S64 line_range =
    {
        ED_LineNumFromPixelOffInCache_GroundTruth(view_range.min, cache),
        ED_LineNumFromPixelOffInCache_GroundTruth(view_range.max, cache),
    };
    
    // rjf: update cached line layouts for visible lines
    for(S64 line_num = line_range.min;
        line_num <= line_range.max && line_num < cache->line_count;
        line_num += 1)
    {
        M_Temp temp = M_BeginTemp(scratch.arena);
        String8 line_string = ED_StringFromBufferLineNum(temp.arena, buffer, line_num);
        Rng1U64List wrapped_ranges = ED_WrappedRangeListFromString(temp.arena, line_string, font, width_constraint);
        
        // rjf: find line layout for this line, or make it if we need to
        ED_LineLayout *line_layout = ED_LineLayoutFromTLineNumInCache(line_num, cache);
        
        // rjf: make spot for line layout, if needed
        if(line_layout == 0 && wrapped_ranges.count > 1)
        {
            U64 insert_idx = 0;
            for(;insert_idx < cache->line_layout_count; insert_idx += 1)
            {
                if(line_num <= cache->line_layouts[insert_idx].line_num)
                {
                    break;
                }
            }
            
            // rjf: grow line layout storage if needed
            if(cache->line_layout_count + 1 > cache->line_layout_cap)
            {
                U64 new_cap = cache->line_layout_cap<<1;
                new_cap = ClampBot(64, new_cap);
                U64 cap_delta = new_cap - cache->line_layout_cap;
                ED_LineLayout *push_base = PushArrayZero(cache->line_layout_arena, ED_LineLayout, cap_delta);
                if(cache->line_layouts == 0)
                {
                    cache->line_layouts = push_base;
                }
                cache->line_layout_cap = new_cap;
            }
            
            // rjf: shift any succeeding line layouts back
            if(insert_idx < cache->line_layout_count)
            {
                MemoryMove(cache->line_layouts + insert_idx + 1,
                           cache->line_layouts + insert_idx,
                           sizeof(ED_LineLayout) * (cache->line_layout_count - insert_idx));
            }
            
            // rjf: get line layout
            line_layout = cache->line_layouts + insert_idx;
            cache->line_layout_count += 1;
            MemoryZeroStruct(line_layout);
        }
        
        // rjf: free all existing wrapped ranges
        if(line_layout != 0)
        {
            for(Rng1U64Node *n = line_layout->wrap_ranges.first, *next = 0; n != 0; n = next)
            {
                next = n->next;
                n->next = cache->first_free_rng_node;
                cache->first_free_rng_node = n;
            }
            MemoryZeroStruct(&line_layout->wrap_ranges);
        }
        
        // rjf: free line layout, if needed
        if(line_layout != 0 && wrapped_ranges.count <= 1)
        {
            U64 idx = line_layout - cache->line_layouts;
            MemoryMove(cache->line_layouts + idx, cache->line_layouts + idx + 1,
                       sizeof(ED_LineLayout) * (cache->line_layout_count - (idx+1)));
            cache->line_layout_count -= 1;
        }
        
        // rjf: store wrapped ranges, and fill line layout info, if nontrivial
        if(line_layout != 0 && wrapped_ranges.count > 1)
        {
            for(Rng1U64Node *n = wrapped_ranges.first; n != 0; n = n->next)
            {
                Rng1U64Node *store = cache->first_free_rng_node;
                if(store)
                {
                    cache->first_free_rng_node = cache->first_free_rng_node->next;
                }
                else
                {
                    store = PushArrayZero(cache->arena, Rng1U64Node, 1);
                }
                store->v = n->v;
                Rng1U64ListPushNode(&line_layout->wrap_ranges, store);
            }
            
            line_layout->line_num = line_num;
            line_layout->scale = line_layout->wrap_ranges.count;
        }
        
        M_EndTemp(temp);
    }
    
    // rjf: calculate height
    cache->total_height = cache->line_count * cache->line_height;
    for(U64 idx = 0; idx < cache->line_layout_count; idx += 1)
    {
        cache->total_height += (cache->line_layouts[idx].scale - 1) * cache->line_height;
    }
    
    ReleaseScratch(scratch);
}

function Rng1U64List
ED_WrappedRangeListFromString(M_Arena *arena, String8 line_str, R_Font font, F32 width_constraint)
{
    Rng1U64List list = {0};
    F32 off = 0;
    U64 wrap_start = 0;
    for(U64 idx = 0, next_idx = line_str.size+1; idx < line_str.size; idx = next_idx)
    {
        next_idx = idx+1;
        if(!CharIsSpace(line_str.str[idx]))
        {
            U64 start_idx = idx;
            U64 end_idx = start_idx+1;
            for(;end_idx <= line_str.size; end_idx += 1)
            {
                if(end_idx == line_str.size || CharIsSpace(line_str.str[end_idx]))
                {
                    break;
                }
            }
            String8 additional_chunk = Substr8(line_str, R1U64(start_idx, end_idx));
            F32 advance = R_AdvanceFromString(font, additional_chunk).x;
            if(off + advance >= width_constraint)
            {
                Rng1U64ListPush(arena, &list, R1U64(wrap_start, start_idx-1));
                wrap_start = start_idx;
                off = 0;
                next_idx = end_idx+1;
            }
        }
        
        off += R_AdvanceFromString(font, Substr8(line_str, R1U64(idx, next_idx))).x;
    }
    if(wrap_start < line_str.size && wrap_start != 0)
    {
        Rng1U64ListPush(arena, &list, R1U64(wrap_start, line_str.size));
    }
    return list;
}

function U64
ED_TotalHeightFromTextLayoutCache(ED_TextLayoutCache *cache)
{
    return cache->total_height;
}

function S64
ED_LineNumFromPixelOffInCache_GroundTruth(F32 pixel_off, ED_TextLayoutCache *cache)
{
    S64 result = (pixel_off / cache->line_height) + 1;
    if(cache->line_layout_count != 0)
    {
        S64 last_line_num = 1;
        F32 off = 0;
        for(U64 ll_idx = 0; ll_idx < cache->line_layout_count; ll_idx += 1)
        {
            ED_LineLayout *ll = &cache->line_layouts[ll_idx];
            S64 line_delta = ll->line_num - last_line_num;
            off += line_delta * cache->line_height;
            
            if(pixel_off < off)
            {
                break;
            }
            else if(off <= pixel_off && pixel_off < off + cache->line_height*ll->scale)
            {
                result = ll->line_num;
                break;
            }
            else if(off < pixel_off)
            {
                off += ll->scale * cache->line_height;
                result = ll->line_num+1 + (S64)((pixel_off-off)/cache->line_height);
                last_line_num = ll->line_num+1;
            }
        }
    }
    result = Clamp(1, result, cache->line_count);
    return result;
}

function Rng1F32
ED_PixelRangeFromLineNumInCache_GroundTruth(S64 line_num, ED_TextLayoutCache *cache)
{
    F32 line_height = cache->line_height;
    F32 off = (line_num-1) * line_height;
    {
        S64 last_line_num = 1;
        F32 base_line_off = 0;
        for(U64 ll_idx = 0; ll_idx < cache->line_layout_count; ll_idx += 1)
        {
            ED_LineLayout *layout = &cache->line_layouts[ll_idx];
            if(layout->line_num <= line_num)
            {
                S64 line_delta = layout->line_num - last_line_num;
                last_line_num = layout->line_num + 1;
                base_line_off += line_delta * cache->line_height;
                off = base_line_off + ClampBot(0, (line_num - (layout->line_num+1))) * cache->line_height;
                if(layout->line_num == line_num)
                {
                    line_height *= layout->scale;
                    break;
                }
                else
                {
                    off += layout->scale*cache->line_height;
                    base_line_off += layout->scale*cache->line_height;
                }
            }
            else
            {
                break;
            }
        }
    }
    return R1F32(off, off+line_height);
}

function ED_LineLayout *
ED_LineLayoutFromTLineNumInCache(S64 tline_num, ED_TextLayoutCache *cache)
{
    ED_LineLayout *result = 0;
    for(U64 idx = 0; idx < cache->line_layout_count; idx += 1)
    {
        if(cache->line_layouts[idx].line_num == tline_num)
        {
            result = &cache->line_layouts[idx];
            break;
        }
    }
    return result;
}

function Rng1U64List
ED_VRangeListFromTLineNumInCache(M_Arena *arena, S64 tline_num, ED_Buffer *buffer, ED_TextLayoutCache *cache)
{
    Rng1U64List result = {0};
    ED_LineLayout *layout = ED_LineLayoutFromTLineNumInCache(tline_num, cache);
    Rng1U64List *cached = 0;
    if(layout != 0)
    {
        cached = &layout->wrap_ranges;
    }
    if(cached != 0)
    {
        result = *cached;
    }
    else
    {
        Rng1U64ListPush(arena, &result, R1U64(0, ED_SizeFromBufferLineNum(buffer, tline_num)));
    }
    return result;
}

////////////////////////////////
//~ rjf: Text Controls

function TE_Op
ED_SingleLineOpFromInputAction(M_Arena *arena, TE_InputAction action, TE_Point cursor, TE_Point mark, S64 preferred_column, ED_Buffer *buffer, ED_TextLayoutCache *cache)
{
    M_Temp scratch = GetScratch(&arena, 1);
    String8 line = ED_StringFromBufferLineNum(scratch.arena, buffer, cursor.line+0);
    
    // rjf: get info about visual line the cursor is on
    Rng1S64 enclosing_wrap_range = {0};
    if(action.flags & TE_InputActionFlag_VisualLines)
    {
        Rng1U64List wrap_ranges = ED_VRangeListFromTLineNumInCache(scratch.arena, cursor.line, buffer, cache);
        for(Rng1U64Node *rng_n = wrap_ranges.first; rng_n != 0; rng_n = rng_n->next)
        {
            Rng1S64 col_rng = {rng_n->v.min+1, rng_n->v.max+1};
            if(col_rng.min <= cursor.column && cursor.column <= col_rng.max)
            {
                enclosing_wrap_range = col_rng;
                break;
            }
        }
    }
    
    // rjf: map textual => visual if we have the visual flag marked
    if(action.flags & TE_InputActionFlag_VisualLines)
    {
        cursor.column = (cursor.column - enclosing_wrap_range.min) + 1;
        mark.column = (mark.column - enclosing_wrap_range.min) + 1;
        line = Substr8(line, R1U64(enclosing_wrap_range.min-1, enclosing_wrap_range.max-1));
    }
    
    // rjf: do single-line op in base layer
    TE_Op op = TE_SingleLineOpFromInputAction(arena, action, line, cursor, mark);
    
    // rjf: re-transform result back to textual coordinates if we have the visual flag marked
    if(action.flags & TE_InputActionFlag_VisualLines)
    {
        op.range.min.column = op.range.min.column + (enclosing_wrap_range.min-1);
        op.range.max.column = op.range.max.column + (enclosing_wrap_range.min-1);
        op.cursor.column = op.cursor.column + (enclosing_wrap_range.min-1);
        op.mark.column = op.mark.column + (enclosing_wrap_range.min-1);
        op.new_preferred_column = op.new_preferred_column + (enclosing_wrap_range.min-1);
    }
    
    ReleaseScratch(scratch);
    return op;
}

function TE_Op
ED_MultiLineOpFromInputAction(M_Arena *arena, TE_InputAction action, TE_Point cursor, TE_Point mark, S64 preferred_column, ED_Buffer *buffer, ED_TextLayoutCache *cache)
{
    M_Temp scratch = GetScratch(&arena, 1);
    
    B32 taken = 0;
    TE_Point next_cursor = cursor;
    TE_Point next_mark = mark;
    S64 next_preferred_column = preferred_column;
    
    //- rjf: horizontal movement
    {
        TE_Range cursor_prev_line_range = TE_RangeMake(TE_PointMake(cursor.line-1, 1), TE_PointMake(cursor.line-1, ED_SizeFromBufferLineNum(buffer, cursor.line-1)+1));
        TE_Range cursor_line_range      = TE_RangeMake(TE_PointMake(cursor.line+0, 1), TE_PointMake(cursor.line+0, ED_SizeFromBufferLineNum(buffer, cursor.line+0)+1));
        TE_Range cursor_next_line_range = TE_RangeMake(TE_PointMake(cursor.line+1, 1), TE_PointMake(cursor.line+1, ED_SizeFromBufferLineNum(buffer, cursor.line+1)+1));
        
        //- rjf: wrap cursor right
        if(action.delta.x > 0 && cursor.column >= cursor_line_range.max.column)
        {
            taken = 1;
            next_cursor = cursor_next_line_range.min;
            next_preferred_column = next_cursor.column;
        }
        
        //- rjf: wrap cursor left
        if(action.delta.x < 0 && cursor.column <= 1)
        {
            taken = 1;
            next_cursor = cursor_prev_line_range.max;
            next_preferred_column = next_cursor.column;
        }
    }
    
    //- rjf: vertical movement
    {
        S32 moves_left = 0;
        S32 move_delta = 0;
        if(action.delta.y < 0)
        {
            moves_left = -action.delta.y;
            move_delta = -1;
            taken = 1;
        }
        if(action.delta.y > 0)
        {
            moves_left = +action.delta.y;
            move_delta = +1;
            taken = 1;
        }
        
        for(; moves_left > 0; moves_left -= 1)
        {
            S64 pref_column = next_preferred_column;
            TE_Range cursor_prev_line_range = TE_RangeMake(TE_PointMake(next_cursor.line-1, 1), TE_PointMake(next_cursor.line-1, ED_SizeFromBufferLineNum(buffer, next_cursor.line-1)+1));
            TE_Range cursor_line_range      = TE_RangeMake(TE_PointMake(next_cursor.line+0, 1), TE_PointMake(next_cursor.line+0, ED_SizeFromBufferLineNum(buffer, next_cursor.line+0)+1));
            TE_Range cursor_next_line_range = TE_RangeMake(TE_PointMake(next_cursor.line+1, 1), TE_PointMake(next_cursor.line+1, ED_SizeFromBufferLineNum(buffer, next_cursor.line+1)+1));
            
            // rjf: change ranges to visual ranges, if we're in visual-lines mode
            B32 has_prev_range = 0;
            B32 has_next_range = 0;
            if(action.flags & TE_InputActionFlag_VisualLines)
            {
                S64 enclosing_prev_line = next_cursor.line;
                Rng1S64 enclosing_prev_range = {0};
                Rng1S64 enclosing_range = {0};
                S64 enclosing_next_line = next_cursor.line;
                Rng1S64 enclosing_next_range = {0};
                {
                    Rng1U64List wrap_ranges = ED_VRangeListFromTLineNumInCache(scratch.arena, next_cursor.line, buffer, cache);
                    Rng1U64 prev_range = {0};
                    for(Rng1U64Node *n = wrap_ranges.first;
                        n != 0;
                        prev_range = n->v, n = n->next)
                    {
                        Rng1S64 col_rng = R1S64(n->v.min+1, n->v.max+1);
                        if(col_rng.min <= next_cursor.column && next_cursor.column <= col_rng.max)
                        {
                            enclosing_prev_range = R1S64(prev_range.min+1, prev_range.max+1);
                            enclosing_range = col_rng;
                            if(n->next)
                            {
                                has_next_range = 1;
                                enclosing_next_range = R1S64(n->next->v.min+1, n->next->v.max+1);
                            }
                            break;
                        }
                        has_prev_range = 1;
                    }
                    
                    if(has_prev_range == 0)
                    {
                        Rng1U64List wrap_ranges = ED_VRangeListFromTLineNumInCache(scratch.arena, next_cursor.line-1, buffer, cache);
                        has_prev_range = 1;
                        enclosing_prev_line = next_cursor.line-1;
                        enclosing_prev_range = R1S64(wrap_ranges.last->v.min+1, wrap_ranges.last->v.max+1);
                    }
                    
                    if(has_next_range == 0)
                    {
                        Rng1U64List wrap_ranges = ED_VRangeListFromTLineNumInCache(scratch.arena, next_cursor.line+1, buffer, cache);
                        has_next_range = 1;
                        enclosing_next_line = next_cursor.line+1;
                        enclosing_next_range = R1S64(wrap_ranges.first->v.min+1, wrap_ranges.first->v.max+1);
                    }
                }
                
                // rjf: prev line
                if(has_prev_range)
                {
                    cursor_prev_line_range.min.line = enclosing_prev_line;
                    cursor_prev_line_range.min.column = enclosing_prev_range.min;
                    cursor_prev_line_range.max.line = enclosing_prev_line;
                    cursor_prev_line_range.max.column = enclosing_prev_range.max;
                }
                
                // rjf: next line
                if(has_next_range)
                {
                    cursor_next_line_range.min.line = enclosing_next_line;
                    cursor_next_line_range.min.column = enclosing_next_range.min;
                    cursor_next_line_range.max.line = enclosing_next_line;
                    cursor_next_line_range.max.column = enclosing_next_range.max;
                }
                
                // rjf: current line
                {
                    cursor_line_range.min.column = enclosing_range.min;
                    cursor_line_range.max.column = enclosing_range.max;
                    pref_column = (pref_column - enclosing_range.min) + 1;
                }
            }
            
            // rjf: apply move to cursor
            B32 moved_visual_lines = 0;
            S64 next_visual_move_preferred_column = pref_column;
            if(move_delta < 0)
            {
                moved_visual_lines = has_prev_range;
                next_cursor.line = cursor_prev_line_range.min.line;
                next_cursor.column = cursor_prev_line_range.min.column + pref_column - 1;
                if(moved_visual_lines)
                {
                    next_preferred_column = next_cursor.column;
                }
                next_cursor.column = Clamp(1, next_cursor.column, cursor_prev_line_range.max.column);
            }
            else
            {
                moved_visual_lines = has_next_range;
                next_cursor.line = cursor_next_line_range.min.line;
                next_cursor.column = cursor_next_line_range.min.column + pref_column - 1;
                if(moved_visual_lines)
                {
                    next_preferred_column = next_cursor.column;
                }
                next_cursor.column = Clamp(1, next_cursor.column, cursor_next_line_range.max.column);
            }
        }
    }
    
    //- rjf: turn action's insertion codepoint into a replace string, if it's a newline
    String8 replace = {0};
    TE_Range range = {0};
    S64 line_delta = 0;
    if(action.codepoint == '\n')
    {
        taken = 1;
        replace = PushStr8Copy(arena, Str8Lit("\n"));
        range = TE_RangeMake(cursor, mark);
        next_cursor = next_mark = TE_PointMake(TE_MinPoint(cursor, mark).line+1, 1);
        line_delta = 1;
    }
    
    //- rjf: form deletion range
    if(action.flags & TE_InputActionFlag_Delete)
    {
        range = TE_RangeMake(next_cursor, next_mark);
        next_cursor = next_mark = TE_MinPoint(next_cursor, next_mark);
    }
    
    //- rjf: stick mark to cursor
    if(!(action.flags & TE_InputActionFlag_KeepMark))
    {
        next_mark = next_cursor;
    }
    
    //- rjf: clamp next cursor/mark
    {
        TE_Range buffer_range = ED_TextRangeFromBuffer(buffer);
        if(line_delta != 0)
        {
            buffer_range.max = TE_PointMake(buffer_range.max.line+1, 1);
        }
        if(TE_PointLessThan(next_cursor, buffer_range.min))
        {
            next_cursor = buffer_range.min;
        }
        else if(TE_PointLessThan(buffer_range.max, next_cursor))
        {
            next_cursor = buffer_range.max;
        }
        if(TE_PointLessThan(next_mark, buffer_range.min))
        {
            next_mark = buffer_range.min;
        }
        else if(TE_PointLessThan(buffer_range.max, next_mark))
        {
            next_mark = buffer_range.max;
        }
    }
    
    //- rjf: fill op + return
    TE_Op op = {0};
    {
        op.taken = taken;
        op.replace = replace;
        op.range = range;
        op.cursor = next_cursor;
        op.mark = next_mark;
        op.new_preferred_column = next_preferred_column;
        op.copy = 0;
        op.paste = 0;
    }
    ReleaseScratch(scratch);
    return op;
}

////////////////////////////////
//~ rjf: Syntax Highlighting

function Vec4F32
ED_ThemeColorFromTokenKind(TM_Theme *theme, ED_TokenKind kind)
{
    Vec4F32 result = result = TM_ThemeColorFromString(theme, Str8Lit("code_plain"));
    switch(kind)
    {
        default:break;
        case ED_TokenKind_Keyword:        {result = TM_ThemeColorFromString(theme, Str8Lit("code_keyword"));}break;
        case ED_TokenKind_NumericLiteral: {result = TM_ThemeColorFromString(theme, Str8Lit("code_numeric_literal"));}break;
        case ED_TokenKind_StringLiteral:  {result = TM_ThemeColorFromString(theme, Str8Lit("code_string_literal"));}break;
        case ED_TokenKind_CharLiteral:    {result = TM_ThemeColorFromString(theme, Str8Lit("code_char_literal"));}break;
        case ED_TokenKind_Operator:       {result = TM_ThemeColorFromString(theme, Str8Lit("code_operator"));}break;
        case ED_TokenKind_Delimiter:      {result = TM_ThemeColorFromString(theme, Str8Lit("code_delimiter"));}break;
        case ED_TokenKind_Comment:        {result = TM_ThemeColorFromString(theme, Str8Lit("code_comment"));}break;
        case ED_TokenKind_Preprocessor:   {result = TM_ThemeColorFromString(theme, Str8Lit("code_preprocessor"));}break;
        case ED_TokenKind_ScopeOpen:
        case ED_TokenKind_ScopeClose:
        case ED_TokenKind_ParenOpen:
        case ED_TokenKind_ParenClose:
        {result = TM_ThemeColorFromString(theme, Str8Lit("code_delimiter"));}break;
        case ED_TokenKind_BlockCommentOpen:
        case ED_TokenKind_BlockCommentClose:
        {result = TM_ThemeColorFromString(theme, Str8Lit("code_comment"));}break;
        case ED_TokenKind_MultiLineStringLiteralOpen:
        case ED_TokenKind_MultiLineStringLiteralClose:
        {result = TM_ThemeColorFromString(theme, Str8Lit("code_string_literal"));}break;
    }
    return result;
}

////////////////////////////////
//~ rjf: UI Helpers

function UI_InteractResult
CW_CodeEdit(B32 keyboard_focused, CW_State *state, C_View *view, C_Entity *entity, Rng1S64 line_range, F32 space_for_margin, TM_Theme *theme, String8 find_string, String8 string)
{
    M_Temp scratch = GetScratch(0, 0);
    C_TextLayoutCache *tl_cache = view->text_layout_cache;
    C_LangRuleSet *lang_rules = C_LangRuleSetFromEntity(entity);
    
    //- rjf: get config data
    MD_Node *comment_highlights_cfg        = CFG_NodeFromKey(Str8Lit("comment_highlights"), 1);
    MD_Node *cursor_style_cfg              = CFG_NodeFromKey(Str8Lit("cursor_style"), 0);
    MD_Node *cursor_blink_style_cfg        = CFG_NodeFromKey(Str8Lit("cursor_blink_style"), 0);
    MD_Node *cursor_corner_radius_cfg      = CFG_NodeFromKey(Str8Lit("cursor_corner_radius"), 0);
    MD_Node *cursor_animation_strength_cfg = CFG_NodeFromKey(Str8Lit("cursor_animation_strength"), 0);
    F32 cursor_corner_radius = (F32)F64FromStr8(Str8FromMD(cursor_corner_radius_cfg->first_child->string));
    F32 cursor_animation_strength = (F32)F64FromStr8(Str8FromMD(cursor_animation_strength_cfg->first_child->string));
    
    //- rjf: get rectangles for all cursors
    U64 cursor_rect_count = view->cursor_states.count;
    Rng2F32 **cursor_rects = PushArrayZero(scratch.arena, Rng2F32 *, cursor_rect_count);
    {
        VIN_Literal default_val = {.rng2f32 = {0}};
        for(U64 idx = 0; idx < cursor_rect_count; idx += 1)
        {
            cursor_rects[idx] = VIN_ValF(Rng2F32, default_val, "cursor_rect_%p_%i", view, (int)idx);
        }
    }
    
    //- rjf: set up UI parameters
    R_Font font = APP_DefaultFont();
    UI_PushCornerRadius(0.f);
    
    //- rjf: begin row
    UI_PushPrefHeight(UI_ChildrenSum(1.f));
    UI_Box *row = UI_BoxMake(0, Str8Lit(""));
    UI_BoxEquipChildLayoutAxis(row, Axis2_X);
    UI_PushParent(row);
    UI_BoxInteract(row);
    UI_PopPrefHeight();
    
    //- rjf: build container widget for text, which will ground all non-gutter user interaction
    UI_PushPrefHeight(UI_ChildrenSum(1.f));
    UI_Box *container = UI_BoxMake(UI_BoxFlag_Clickable, string);
    UI_PopPrefHeight();
    UI_BoxEquipChildLayoutAxis(container, Axis2_Y);
    UI_BoxEquipHoverCursor(container, OS_CursorKind_IBar);
    UI_InteractResult interact = UI_BoxInteract(container);
    UI_PushParent(container);
    
    //- rjf: fill out lines
    S64 mouse_over_line_num = 0;
    UI_Box *mouse_over_line_box = &ui_g_nil_box;
    U64 mouse_over_vline_base = 0;
    {
        for(S64 line_num = line_range.min;
            line_num <= line_range.max;
            line_num += 1)
        {
            //- rjf: get line info
            String8 tline_string = C_StringFromBufferLineNum(scratch.arena, entity->buffer, line_num);
            C_TokenList tline_tokens = C_TokenListFromBufferLineNum(lang_rules, entity->buffer, line_num);
            F32 tline_height = Dim1F32(C_PixelRangeFromLineNumInCache_GroundTruth(line_num, tl_cache));
            Rng1U64List vline_ranges = C_VRangeListFromTLineNumInCache(scratch.arena, line_num, entity->buffer, tl_cache);
            B32 line_in_range = 0;
            for(C_CursorState *cs = view->cursor_states.first; cs != 0; cs = cs->next)
            {
                TE_Point cursor = C_CursorFromCursorState(cs);
                TE_Point mark = C_MarkFromCursorState(cs);
                TE_Range range = TE_RangeMake(cursor, mark);
                if(range.min.line <= line_num && line_num <= range.max.line)
                {
                    line_in_range = 1;
                    break;
                }
            }
            
            //- rjf: build vline tokens lists
            C_TokenList *vline_tokens = PushArrayZero(scratch.arena, C_TokenList, vline_ranges.count);
            {
                U64 vline_idx = 0;
                C_TokenNode *token_n = tline_tokens.first;
                Rng1U64Node *vline_rng_n = vline_ranges.first;
                for(;token_n != 0 && vline_rng_n != 0;)
                {
                    Rng1U64 vline_range = vline_rng_n->v;
                    C_Token tline_token = token_n->token;
                    
                    // rjf: if this token overlaps the vline, we need to add a
                    // corresponding token
                    B32 token_done = 0;
                    if(vline_range.min <= tline_token.range.max && tline_token.range.min < vline_range.max)
                    {
                        C_Token token = {0};
                        token.kind = tline_token.kind;
                        token.range.min = Max(tline_token.range.min, vline_range.min);
                        token.range.max = Min(tline_token.range.max, vline_range.max);
                        token.range.min -= vline_range.min;
                        token.range.max -= vline_range.min;
                        C_TokenListPush(scratch.arena, &vline_tokens[vline_idx], token);
                        token_done = tline_token.range.max <= vline_range.max;
                    }
                    
                    // rjf: advance
                    if(token_done)
                    {
                        token_n = token_n->next;
                    }
                    else
                    {
                        vline_rng_n = vline_rng_n->next;
                        vline_idx += 1;
                    }
                }
            }
            
            //- rjf: begin row for line
            UI_Box *line_row = &ui_g_nil_box;
            UI_PrefHeight(UI_Pixels(tline_height, 1.f)) line_row = UI_RowBegin().box;
            
            //- rjf: line number
            UI_Box *line_num_box = &ui_g_nil_box;
            UI_InteractResult line_num_interact = {0};
            UI_PrefWidth(UI_Pixels(space_for_margin, 1.f))
                UI_PrefHeight(UI_Pixels(tline_height, 1.f))
                UI_TextColor(line_in_range
                             ? theme->colors[TM_ColorCode_CodeLineNumberActive]
                             : theme->colors[TM_ColorCode_CodeLineNumberInactive])
            {
                line_num_box = UI_BoxMakeF(UI_BoxFlag_DrawText, "%" PRIi64 "##_line_num_%p", line_num, entity);
                line_num_interact = UI_BoxInteract(line_num_box);
            }
            
            //- rjf: begin column for visual lines, if we have more than one
            if(vline_ranges.count > 1)
            {
                UI_ColumnBegin();
            }
            
            //- rjf: build UI for each visual line
            UI_TextColor(theme->colors[TM_ColorCode_CodePlain])
                UI_PrefWidth(UI_TextDim(1, 1))
                UI_PrefHeight(UI_Pixels(font.line_advance, 1.f))
            {
                U64 vline_idx = 0;
                for(Rng1U64Node *n = vline_ranges.first; n != 0; n = n->next, vline_idx += 1)
                {
                    String8 vline_string = Substr8(tline_string, n->v);
                    String8 vline_id_string = PushStr8F(scratch.arena, "%S_line_%" PRIi64 "_%" PRIu64 "",
                                                        string,
                                                        line_num,
                                                        vline_idx);
                    UI_Box *vline_box = UI_BoxMake(UI_BoxFlag_DrawText|UI_BoxFlag_UseFullString, vline_id_string);
                    UI_BoxEquipString(vline_box, vline_string);
                    UI_InteractResult vline_interact = UI_BoxInteract(vline_box);
                    
                    // rjf: syntax highlight
                    C_TokenList tokens = vline_tokens[vline_idx];
                    if(tokens.count != 0)
                    {
                        // rjf: build colored ranges
                        ColoredRange1U64List colored_rngs = {0};
                        for(C_TokenNode *n = tokens.first; n != 0; n = n->next)
                        {
                            Vec4F32 color = CW_ThemeColorFromTokenKind(theme, n->token.kind);
                            Rng1U64 range = n->token.range;
                            ColoredRange1U64ListPush(scratch.arena, &colored_rngs, ColoredRange1U64Make(color, range));
                        }
                        
                        // rjf: upgrade with comment highlights
                        if(!MD_NodeIsNil(comment_highlights_cfg))
                        {
                            for(C_TokenNode *n = tokens.first; n != 0; n = n->next)
                            {
                                if(n->token.kind == C_TokenKind_Comment)
                                {
                                    for(MD_EachNode(highlight, comment_highlights_cfg->first_child))
                                    {
                                        String8 highlight_str = Str8FromMD(highlight->first_child->string);
                                        String8 color_str = Str8FromMD(highlight->first_child->next->string);
                                        Vec4F32 highlight_color = theme->colors[TM_ColorCodeFromStringLower(color_str)];
                                        String8 token_str = Substr8(vline_string, n->token.range);
                                        for(U64 start = 0; start < token_str.size;)
                                        {
                                            U64 pos_in_token = FindSubstr8(token_str, highlight_str, start, 0);
                                            start = pos_in_token + highlight_str.size;
                                            if(pos_in_token < token_str.size)
                                            {
                                                ColoredRange1U64ListPush(scratch.arena, &colored_rngs, ColoredRange1U64Make(highlight_color, R1U64(n->token.range.min+pos_in_token, n->token.range.min+pos_in_token+highlight_str.size)));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // rjf: convert to fancy string list
                        FancyStringList fancy_strings = FancyStringListFromStringAndColoredRangeList(UI_FrameArena(), PushStr8Copy(UI_FrameArena(), vline_string),
                                                                                                     colored_rngs, theme->colors[TM_ColorCode_CodePlain]);
                        
                        // rjf: fill box
                        vline_box->flags &= ~UI_BoxFlag_DrawText;
                        vline_box->flags |= UI_BoxFlag_DrawFancyText;
                        UI_BoxEquipFancyStringList(vline_box, fancy_strings);
                    }
                    
                    // rjf: do extra drawing
                    {
                        R_Font font = vline_box->font;
                        DR_Bucket line_drl = {0};
                        DR_Bucket line_num_drl = {0};
                        Vec2F32 text_position = UI_BoxTextPosition(vline_box);
                        Rng1U64 vline_range = n->v;
                        
                        // rjf: per-cursor-mark-pair extra drawing
                        U64 cursor_idx = 0;
                        for(C_CursorState *cs = view->cursor_states.first;
                            cs != 0;
                            cs = cs->next, cursor_idx += 1)
                        {
                            TE_Point cursor = C_CursorFromCursorState(cs);
                            TE_Point mark = C_MarkFromCursorState(cs);
                            F32 cursor_pixel_off = R_AdvanceFromString(font, Prefix8(vline_string, cursor.column-vline_range.min-1)).x;
                            TE_Range cs_range = TE_RangeMake(mark, cursor);
                            
                            // rjf: extra rendering for cursor border
                            if(cursor.line == line_num)
                            {
                                line_row->flags |= UI_BoxFlag_DrawBorder;
                                line_row->border_color = V4F32(1, 1, 1, 0.1f);
                            }
                            
                            // rjf: extra rendering for cursor
                            if(cursor.line == line_num &&
                               vline_range.min <= cursor.column-1 && cursor.column-1 <= vline_range.max)
                            {
                                String8 cursor_style = Str8FromMD(cursor_style_cfg->first_child->string);
                                String8 cursor_blink_style = Str8FromMD(cursor_blink_style_cfg->first_child->string);
                                
                                F32 blink_f = cosf(0.5f * 3.14159f * UI_CaretBlinkT() / OS_CaretBlinkTime());
                                blink_f = blink_f * blink_f;
                                Vec4F32 cursor_color = theme->colors[(cursor_idx == 0 || state->mode_flags & CW_ModeFlag_MultiCursor) ? TM_ColorCode_CodeCursor : TM_ColorCode_CodeCursorAlt];
                                if(keyboard_focused == 0)
                                {
                                    blink_f = 0.2f;
                                }
                                
                                // rjf: fade blinking
                                if(Str8Match(cursor_blink_style, Str8Lit("fade"), MatchFlag_CaseInsensitive))
                                {
                                    cursor_color.a = 0.8*blink_f;
                                }
                                // rjf: flash
                                else if(Str8Match(cursor_blink_style, Str8Lit("flash"), MatchFlag_CaseInsensitive))
                                {
                                    cursor_color.a = blink_f > 0.5f ? 1 : 0.1f;
                                }
                                // rjf: no blinking
                                else{}
                                
                                // rjf: get cursor rect
                                Rng2F32 target_cursor_rect = {0};
                                {
                                    // rjf: insert-mode cursor (underline)
                                    if(state->mode_flags & CW_ModeFlag_Insert)
                                    {
                                        R_Glyph space_glyph = R_GlyphFromFontCodepoint(font, ' ');
                                        target_cursor_rect =
                                        (Rng2F32)
                                        {
                                            cursor_pixel_off,
                                            vline_box->rect.y1 - 2.f,
                                            space_glyph.advance + cursor_pixel_off,
                                            vline_box->rect.y1,
                                        };
                                    }
                                    
                                    // rjf: block-style cursor
                                    else if(Str8Match(cursor_style, Str8Lit("block"), MatchFlag_CaseInsensitive))
                                    {
                                        R_Glyph space_glyph = R_GlyphFromFontCodepoint(font, ' ');
                                        target_cursor_rect =
                                        (Rng2F32)
                                        {
                                            cursor_pixel_off,
                                            vline_box->rect.y0,
                                            space_glyph.advance + cursor_pixel_off,
                                            vline_box->rect.y1,
                                        };
                                    }
                                    
                                    // rjf: line-style cursor
                                    else
                                    {
                                        target_cursor_rect =
                                        (Rng2F32)
                                        {
                                            -1.f + cursor_pixel_off,
                                            vline_box->rect.y0-2.f,
                                            +1.f + cursor_pixel_off,
                                            vline_box->rect.y1+2.f,
                                        };
                                    }
                                }
                                
                                // rjf: animate & draw
                                Rng2F32 *cursor_rect = cursor_rects[cursor_idx];
                                F32 rate = cursor_animation_strength / (APP_DT()/2);
                                F32 slow_rate = rate*0.5f;
                                F32 fast_rate = rate*2.f;
                                F32 ratex0 = target_cursor_rect.x0 < cursor_rect->x0 ? fast_rate : slow_rate;
                                F32 ratex1 = target_cursor_rect.x0 > cursor_rect->x0 ? fast_rate : slow_rate;
                                APP_AnimateF32_Exp(&cursor_rect->x0, target_cursor_rect.x0, ratex0);
                                APP_AnimateF32_Exp(&cursor_rect->y0, target_cursor_rect.y0, rate);
                                APP_AnimateF32_Exp(&cursor_rect->x1, target_cursor_rect.x1, ratex1);
                                APP_AnimateF32_Exp(&cursor_rect->y1, target_cursor_rect.y1, rate);
                                Rng2F32 rect =
                                {
                                    cursor_rect->x0 + text_position.x,
                                    cursor_rect->y0,
                                    cursor_rect->x1 + text_position.x,
                                    cursor_rect->y1,
                                };
                                DR_Rect_C(&line_drl, rect, cursor_color, cursor_corner_radius);
                            }
                            
                            // rjf: extra rendering for selection
                            TE_Range vline_text_range = TE_RangeMake(TE_PointMake(line_num, vline_range.min+1), TE_PointMake(line_num, vline_range.max+1));
                            if(!TE_PointMatch(cs_range.min, cs_range.max) &&
                               (TE_PointLessThan(vline_text_range.min, cs_range.max) || TE_PointMatch(vline_text_range.min, cs_range.max)) &&
                               (TE_PointLessThan(cs_range.min, vline_text_range.max) || TE_PointMatch(cs_range.min, vline_text_range.max)))
                            {
                                Vec4F32 selection_color = theme->colors[TM_ColorCode_BaseTextSelection];
                                if(keyboard_focused == 0)
                                {
                                    selection_color.a /= 2.f;
                                }
                                TE_Point selection_in_line_min = TE_PointLessThan(vline_text_range.min, cs_range.min) ? cs_range.min : vline_text_range.min;
                                TE_Point selection_in_line_max = TE_PointLessThan(cs_range.max, vline_text_range.max) ? cs_range.max : vline_text_range.max;
                                F32 min_pixel_off = R_AdvanceFromString(font, Prefix8(vline_string, selection_in_line_min.column-vline_range.min-1)).x;
                                F32 max_pixel_off = R_AdvanceFromString(font, Prefix8(vline_string, selection_in_line_max.column-vline_range.min-1)).x;
                                Rng2F32 selection_rect =
                                {
                                    text_position.x + min_pixel_off,
                                    vline_box->rect.y0,
                                    text_position.x + max_pixel_off,
                                    vline_box->rect.y1,
                                };
                                DR_Rect(&line_drl, selection_rect, selection_color);
                            }
                        }
                        
                        // rjf: extra rendering for find_str
                        if(find_string.size != 0)
                        {
                            for(U64 idx = 0; idx < vline_string.size;)
                            {
                                U64 find_pos = FindSubstr8(vline_string, find_string, idx, MatchFlag_CaseInsensitive);
                                idx = find_pos + find_string.size;
                                if(find_pos < vline_string.size)
                                {
                                    Vec4F32 color = theme->colors[TM_ColorCode_CodeRangeHighlight];
                                    
                                    B32 cursor_is_on_occurrence = 0;
                                    for(C_CursorState *cs = view->cursor_states.first; cs != 0; cs = cs->next)
                                    {
                                        TE_Point cursor = C_CursorFromCursorState(cs);
                                        if(cursor.line != line_num ||
                                           cursor.column-1 < find_pos ||
                                           find_pos + find_string.size < cursor.column-1)
                                        {
                                            cursor_is_on_occurrence = 1;
                                            break;
                                        }
                                    }
                                    
                                    if(cursor_is_on_occurrence)
                                    {
                                        color.a /= 2.f;
                                    }
                                    
                                    F32 min_pixel_off = R_AdvanceFromString(font, Prefix8(vline_string, find_pos)).x;
                                    F32 max_pixel_off = R_AdvanceFromString(font, Prefix8(vline_string, find_pos+find_string.size)).x;
                                    Rng2F32 find_rect =
                                    {
                                        text_position.x + min_pixel_off,
                                        vline_box->rect.y0,
                                        text_position.x + max_pixel_off,
                                        vline_box->rect.y1,
                                    };
                                    DR_Rect_C(&line_drl, find_rect, color, 4.f);
                                }
                            }
                        }
                        
                        // rjf: (DEBUG) range endpoints
#if 0
                        {
                            Rng1U64 tokenrngpt_range = C_ContributingTokenRangePointRangeFromEntityLineNum_GroundTruth(entity, line_num);
                            for(U64 idx = tokenrngpt_range.min; idx < tokenrngpt_range.max; idx += 1)
                            {
                                String8 prefix = Prefix8(tline_string, entity->token_range_points.v[idx].point.column-1);
                                Vec2F32 v = R_AdvanceFromString(font, prefix);
                                DR_Rect(&line_drl, R2F32(V2F32(text_position.x + v.x,
                                                               text_position.y),
                                                         V2F32(text_position.x + v.x + 4.f,
                                                               text_position.y + 4.f)),
                                        V4F32(1*(entity->token_range_points.v[idx].starter),
                                              1*(!entity->token_range_points.v[idx].starter),
                                              0, 1));
                            }
                        }
#endif
                        
                        // rjf: equip draw buckets, if we drew something
                        if(line_drl.cmds.count != 0)
                        {
                            UI_BoxEquipDrawBucketRelative(vline_box, &line_drl);
                        }
                        if(line_num_drl.cmds.count != 0)
                        {
                            UI_BoxEquipDrawBucketRelative(line_num_box, &line_num_drl);
                        }
                    }
                    
                    //- rjf: grab info if the mouse is hovering here
                    if(vline_box->rect.y0 <= vline_interact.mouse.y &&
                       vline_interact.mouse.y < vline_box->rect.y1)
                    {
                        mouse_over_line_num = line_num;
                        mouse_over_line_box = vline_box;
                        mouse_over_vline_base = n->v.min;
                    }
                    
                }
            }
            
            //- rjf: end column for visual lines, if we have more than one
            if(vline_ranges.count > 1)
            {
                UI_ColumnEnd();
            }
            
            //- rjf: extra space after line text
            UI_PrefHeight(UI_Pixels(0, 0)) UI_Spacer(UI_Pct(1, 0));
            
            //- rjf: end row for line
            UI_RowEnd();
        }
    }
    
    //- rjf: if mouse is out-of-bounds, clamp at range slightly outside of line range
    if(interact.mouse.y <= container->rect.y0)
    {
        mouse_over_line_box = &ui_g_nil_box;
        mouse_over_line_num = line_range.min-4;
    }
    else if(container->rect.y1 - 30 <= interact.mouse.y)
    {
        mouse_over_line_box = &ui_g_nil_box;
        mouse_over_line_num = line_range.max+4;
    }
    
    //- rjf: pop container
    UI_PopParent();
    
    //- rjf: end row
    UI_PopParent();
    
    //- rjf: calculate mouse position in line/column coordinates
    Vec2F32 mouse = interact.mouse;
    TE_Point mouse_point = {0};
    if(mouse_over_line_num != 0)
    {
        mouse_point = TE_PointMake(mouse_over_line_num, mouse_over_vline_base+UI_BoxCharPosFromXY(mouse_over_line_box, mouse)+1);
    }
    
    //- rjf: click + drag
    if(interact.dragging)
    {
        B32 mouse_point_is_valid = mouse_point.line != 0 && mouse_point.column != 0;
        CW_PushCmd(state, cw_g_cmd_kind_string_table[CW_CmdKind_SnapToCursor]);
        if(interact.pressed && mouse_point_is_valid)
        {
            C_ViewSetMark(view, mouse_point);
        }
        UI_ResetCaretBlinkT();
        if(mouse_point_is_valid)
        {
            C_ViewCloseExtraCursorMarkPairs(view);
            C_ViewSetCursor(view, mouse_point);
            C_ViewResetPreferredColumn(view, mouse_point.column);
        }
    }
    
    UI_PopCornerRadius();
    ReleaseScratch(scratch);
    return interact;
}

function UI_InteractResult
CW_CodeEditF(B32 keyboard_focused, CW_State *state, C_View *view, C_Entity *entity, Rng1S64 line_range, F32 space_for_margin, TM_Theme *theme, String8 find_string, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    UI_InteractResult itrct = CW_CodeEdit(keyboard_focused, state, view, entity, line_range, space_for_margin, theme, find_string, string);
    ReleaseScratch(scratch);
    return itrct;
}

////////////////////////////////
//~ rjf: Hooks

exported void *
TickGlobal(M_Arena *arena, void *state_ptr, OS_EventList *events, C_CmdList *cmds)
{
    if(ed_state == 0)
    {
        ed_state = PushArrayZero(arena, ED_Global, 1);
        ed_state->arena = arena;
        ed_state->language_table_size = 256;
        ed_state->language_table = PushArrayZero(arena, ED_LangNode *, ed_state->language_table_size);
    }
    return ed_state;
}

exported void *
TickView(M_Arena *arena, void *state_ptr, APP_Window *window, OS_EventList *events, C_CmdList *cmds, Rng2F32 rect)
{
    //- rjf: initialize per-view state
    ED_View *view = (ED_View *)state_ptr;
    if(view == 0)
    {
        // rjf: register commands
        {
            M_Temp scratch = GetScratch(&arena, 1);
            C_CmdKindInfo *info = PushArrayZero(scratch.arena, C_CmdKindInfo, ED_CmdKind_COUNT-1);
            for(U64 cmd_idx = 1; cmd_idx < ED_CmdKind_COUNT; cmd_idx += 1)
            {
                info[cmd_idx-1].name = ed_g_cmd_kind_string_table[cmd_idx];
                info[cmd_idx-1].description = ed_g_cmd_kind_desc_table[cmd_idx];
            }
            C_RegisterCommands(ED_CmdKind_COUNT-1, info);
            ReleaseScratch(scratch);
        }
        
        // rjf: fill state
        {
            view = PushArrayZero(arena, ED_View, 1);
            view->arena = arena;
        }
    }
    
    return view;
}

exported void
CloseView(void *state_ptr)
{
    ED_View *view = (ED_View *)state_ptr;
    if(view != 0)
    {
        
    }
}