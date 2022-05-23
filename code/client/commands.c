////////////////////////////////
//~ rjf: Command Parser

#if ENGINE
read_only CMD_Node cmd_node_nil =
{
    CMD_NodeKind_Null,
    &cmd_node_nil,
    &cmd_node_nil,
    &cmd_node_nil,
    &cmd_node_nil,
    &cmd_node_nil,
};
#endif

engine_function B32
CMD_NodeIsNil(CMD_Node *n)
{
    return n == 0 || n == &cmd_node_nil;
}

engine_function CMD_Node *
CMD_PushNode(M_Arena *arena, CMD_NodeKind kind, String8 string, Rng1U64 range_in_input)
{
    CMD_Node *node = PushArrayZero(arena, CMD_Node, 1);
    node->kind = kind;
    node->string = PushStr8Copy(arena, string);
    node->range_in_input = range_in_input;
    node->first = node->last = node->next = node->prev = node->parent = &cmd_node_nil;
    return node;
}

engine_function CMD_Node *
CMD_PushChild(CMD_Node *parent, CMD_Node *child)
{
    child->parent = parent;
    DLLPushBack_NPZ(parent->first, parent->last, child, next, prev, CMD_NodeIsNil, CMD_NodeSetNil);
    return child;
}

engine_function CMD_Node *
CMD_ParseNodeFromString(M_Arena *arena, String8 string)
{
    M_Temp scratch = GetScratch(&arena, 1);
    String8 splits[] = { Str8Lit(" ") };
    
    String8List tokens = {0};
    String8List groups = StrSplit8(scratch.arena, string, ArrayCount(splits), splits);
    
    //- rjf: split by important symbols, and build token list
    String8 symbol_splits[] = { Str8Lit("=") };
    for(String8Node *n = groups.first; n != 0; n = n->next)
    {
        String8List words = StrSplit8(scratch.arena, n->string, ArrayCount(symbol_splits), symbol_splits);
        for(String8Node *w_n = words.first; w_n != 0; w_n = w_n->next)
        {
            Str8ListPush(scratch.arena, &tokens, w_n->string);
            if(w_n->next)
            {
                Str8ListPush(scratch.arena, &tokens, Str8Lit("="));
            }
        }
    }
    
    //- rjf: parse out root
    CMD_Node *root = &cmd_node_nil;
    if(tokens.first)
    {
        U64 first_spot = (U64)(tokens.first->string.str - string.str);
        root = CMD_PushNode(arena, CMD_NodeKind_Root, tokens.first->string, R1U64(first_spot, first_spot + tokens.first->string.size));
    }
    
    //- rjf: parse arguments
    if(root && tokens.node_count > 1)
    {
        for(String8Node *n = tokens.first->next, *next = 0; n != 0; n = next)
        {
            next = n->next;
            
            // rjf: build arg
            U64 first_spot = (U64)(n->string.str - string.str);
            CMD_Node *arg = CMD_PushNode(arena, CMD_NodeKind_Argument, n->string, R1U64(first_spot, first_spot + n->string.size));
            CMD_PushChild(root, arg);
            
            // rjf: check for values
            if(n->next && Str8Match(n->next->string, Str8Lit("="), 0))
            {
                for(String8Node *v_n = n->next->next; v_n != 0; v_n = next = v_n->next)
                {
                    String8 val_splits[] = { Str8Lit(",") };
                    String8List values = StrSplit8(scratch.arena, v_n->string, ArrayCount(val_splits), val_splits);
                    for(String8Node *val_n = values.first; val_n != 0; val_n = val_n->next)
                    {
                        U64 first_spot = (U64)(val_n->string.str - string.str);
                        CMD_Node *value = CMD_PushNode(arena, CMD_NodeKind_Value, val_n->string, R1U64(first_spot, first_spot + val_n->string.size));
                        CMD_PushChild(arg, value);
                    }
                    if(!Str8Match(Suffix8(v_n->string, 1), Str8Lit(","), 0))
                    {
                        break;
                    }
                }
            }
        }
    }
    
    ReleaseScratch(scratch);
    return root;
}

engine_function U64
CMD_IndexFromStringTable(String8 command, U64 string_table_count, String8 *string_table)
{
    U64 result = 0;
    M_Temp scratch = GetScratch(0, 0);
    CMD_Node *root = CMD_ParseNodeFromString(scratch.arena, command);
    if(root->string.size)
    {
        for(U64 idx = 1; idx < string_table_count; idx += 1)
        {
            if(Str8Match(root->string, string_table[idx], MatchFlag_CaseInsensitive))
            {
                result = idx;
                break;
            }
        }
    }
    ReleaseScratch(scratch);
    return result;
}
