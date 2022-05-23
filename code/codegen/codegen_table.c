static MD_Map cg_tbl_top_level_node_grid_map    = {0};
static MD_Map cg_tbl_top_level_table_header_map = {0};

static MD_Map cg_tbl_layer_map_gen = {0};
static MD_Map cg_tbl_layer_map_gen_enum = {0};
static MD_Map cg_tbl_layer_map_gen_data = {0};

static MD_String8 cg_tbl_tag__table          = MD_S8LitComp("table");
static MD_String8 cg_tbl_tag__table_gen      = MD_S8LitComp("table_gen");
static MD_String8 cg_tbl_tag__table_gen_enum = MD_S8LitComp("table_gen_enum");
static MD_String8 cg_tbl_tag__table_gen_data = MD_S8LitComp("table_gen_data");
static MD_String8 cg_tbl_tag__engine         = MD_S8LitComp("engine");

static CG_NodeArray
CG_NodeArrayMake(MD_u64 count)
{
    CG_NodeArray result = {0};
    result.count = count;
    result.v = MD_PushArrayZero(cg_arena, MD_Node *, result.count);
    for(MD_u64 idx = 0; idx < result.count; idx += 1)
    {
        result.v[idx] = MD_NilNode();
    }
    return result;
}

static CG_NodeGrid
CG_GridFromNode(MD_Node *node)
{
    CG_NodeGrid grid = {0};
    
    //- rjf: determine dimensions
    MD_u64 row_count = 0;
    MD_u64 column_count = 0;
    {
        for(MD_EachNode(row, node->first_child))
        {
            row_count += 1;
            MD_u64 cell_count_this_row = MD_ChildCountFromNode(row);
            column_count = MD_Max(cell_count_this_row, column_count);
        }
    }
    
    //- rjf: allocate cells / row parents
    {
        grid.cells = CG_NodeArrayMake(row_count * column_count);
        grid.row_parents = CG_NodeArrayMake(row_count);
    }
    
    //- rjf: fill cells
    {
        MD_u64 row_idx = 0;
        for(MD_EachNode(row, node->first_child))
        {
            MD_u64 col_idx = 0;
            grid.row_parents.v[row_idx] = row;
            for(MD_EachNode(cell, row->first_child))
            {
                grid.cells.v[row_idx * column_count + col_idx] = cell;
                col_idx += 1;
            }
            row_idx += 1;
        }
    }
    
    return grid;
}

static CG_TableHeader
CG_TableHeaderFromTag(MD_Node *tag)
{
    CG_TableHeader result = {0};
    result.column_count = MD_ChildCountFromNode(tag);
    result.column_descs = MD_PushArrayZero(cg_arena, CG_ColumnDesc, result.column_count);
    MD_u64 idx = 0;
    for(MD_EachNode(column_node, tag->first_child))
    {
        result.column_descs[idx].kind = CG_ColumnKind_Default;
        result.column_descs[idx].name = column_node->string;
        MD_Node *check_for_tag = MD_TagFromString(column_node, MD_S8Lit("check_for_tag"), 0);
        if(!MD_NodeIsNil(check_for_tag))
        {
            result.column_descs[idx].kind = CG_ColumnKind_CheckForTag;
            result.column_descs[idx].tag_string = check_for_tag->first_child->string;
        }
        idx += 1;
    }
    return result;
}

static MD_u64
CG_RowChildIndexFromColumnName(CG_TableHeader *header, MD_String8 column_name)
{
    MD_u64 result = 0;
    for(MD_u64 idx = 0; idx < header->column_count; idx += 1)
    {
        if(MD_S8Match(header->column_descs[idx].name, column_name, 0))
        {
            break;
        }
        if(header->column_descs[idx].kind == CG_ColumnKind_Default)
        {
            result += 1;
        }
    }
    return result;
}

static MD_i64
CG_TableExprEvaluate_Numeric(CG_ExpandInfo *info, MD_Expr *expr)
{
    MD_i64 result = 0;
    CG_TableOp op = expr->op ? expr->op->op_id : CG_TableOp_Null;
    switch(op)
    {
        case CG_TableOp_Equal:
        case CG_TableOp_IsNotEqual:
        {
            MD_ArenaTemp scratch = MD_GetScratch(0, 0);
            MD_String8List left_strs = {0};
            MD_String8List right_strs = {0};
            CG_TableExprEvaluate_String(info, expr->left, &left_strs);
            CG_TableExprEvaluate_String(info, expr->right, &right_strs);
            MD_String8 left_str = MD_S8ListJoin(scratch.arena, left_strs, 0);
            MD_String8 right_str = MD_S8ListJoin(scratch.arena, right_strs, 0);
            result = MD_S8Match(left_str, right_str, 0);
            if(op == CG_TableOp_IsNotEqual)
            {
                result = !result;
            }
            MD_ReleaseScratch(scratch);
        }break;
        
        case CG_TableOp_BooleanAnd:
        case CG_TableOp_BooleanOr:
        {
            MD_i64 left = CG_TableExprEvaluate_Numeric(info, expr->left);
            MD_i64 right = CG_TableExprEvaluate_Numeric(info, expr->right);
            switch(op)
            {
                case CG_TableOp_BooleanAnd: result = left && right; break;
                case CG_TableOp_BooleanOr:  result = left || right; break;
            }
        }break;
    }
    return result;
}

static void
CG_TableExprEvaluate_String(CG_ExpandInfo *info, MD_Expr *expr, MD_String8List *out)
{
    CG_TableOp op = expr->op ? expr->op->op_id : CG_TableOp_Null;
    switch(op)
    {
        default:
        case CG_TableOp_Null:
        {
            MD_S8ListPush(cg_arena, out, expr->md_node->string);
        }break;
        
        case CG_TableOp_Dot:
        {
            MD_Expr *label_expr = expr->left;
            MD_Expr *column_query_expr = expr->right;
            MD_Node *label_node = label_expr->md_node;
            MD_Node *column_query_node = column_query_expr->md_node;
            MD_String8 label = label_node->string;
            MD_String8 column_query = column_query_node->string;
            MD_b32 column_query_is_by_name  = column_query_node->flags & MD_NodeFlag_Identifier;
            MD_b32 column_query_is_by_index = column_query_node->flags & MD_NodeFlag_Numeric;
            
            // rjf: find which expansion this label refers to, grab its iterator
            CG_ExpandIter *iter = 0;
            for(CG_ExpandIter *it = info->first_expand_iter; it != 0; it = it->next)
            {
                if(MD_S8Match(it->label, label, 0))
                {
                    iter = it;
                    break;
                }
            }
            
            // rjf: error on invalid label
            if(iter == 0)
            {
                MD_PrintMessageFmt(stderr, MD_CodeLocFromNode(label_node), MD_MessageKind_Error, "Expansion label \"%S\" was not found as referring to a valid @expand tag.", label);
            }
            
            // rjf: generate strings from iterator's table
            if(iter != 0)
            {
                CG_NodeGrid *grid = iter->grid;
                CG_TableHeader *header = iter->header;
                MD_Node *row = grid->row_parents.v[iter->idx];
                
                // rjf: grab the cell string given the row & column_query
                MD_String8 cell_string = {0};
                {
                    // NOTE(rjf): by-name index (look into table header)
                    if(column_query_is_by_name && header != 0)
                    {
                        MD_u64 column_idx = 0;
                        CG_ColumnDesc *column = 0;
                        for(MD_u64 col_idx = 0; col_idx < header->column_count; col_idx += 1)
                        {
                            if(MD_S8Match(header->column_descs[col_idx].name, column_query, 0))
                            {
                                column = &header->column_descs[col_idx];
                                column_idx = col_idx;
                                break;
                            }
                        }
                        MD_u64 row_child_idx = CG_RowChildIndexFromColumnName(header, column_query);
                        
                        // rjf: error on invalid column
                        if(column == 0)
                        {
                            MD_PrintMessageFmt(stderr, MD_CodeLocFromNode(column_query_node), MD_MessageKind_Error, "Column query \"%S\" did not map to a valid column for expansion label \"%S\".", column_query, label);
                        }
                        
                        if(column != 0)
                        {
                            switch(column->kind)
                            {
                                default:
                                case CG_ColumnKind_Default:
                                {
                                    MD_Node *cell_node = MD_ChildFromIndex(row, row_child_idx);
                                    cell_string = cell_node->string;
                                    if(MD_S8Match(cell_node->raw_string, MD_S8Lit("/"), 0))
                                    {
                                        cell_string = MD_S8Lit("");
                                    }
                                }break;
                                
                                case CG_ColumnKind_CheckForTag:
                                {
                                    MD_b32 has_tag = MD_NodeHasTag(row, column->tag_string, 0);
                                    cell_string = has_tag ? MD_S8Lit("1") : MD_S8Lit("0");
                                }break;
                            }
                        }
                    }
                    // NOTE(rjf): by-index (grab nth child of row)
                    else if(column_query_is_by_index)
                    {
                        MD_i64 index = MD_CStyleIntFromString(column_query);
                        cell_string = MD_ChildFromIndex(row, index)->string;
                    }
                }
                
                MD_S8ListPush(cg_arena, out, cell_string);
            }
            
        }break;
        
        case CG_TableOp_Bump:
        {
            MD_u64 dst = MD_CStyleIntFromString(expr->unary_operand->md_node->string);
            MD_u64 src = out->total_size;
            MD_u64 spaces_to_print = dst - src;
            if(dst > src)
            {
                for(MD_u64 space_idx = 0; space_idx < spaces_to_print; space_idx += 1)
                {
                    MD_S8ListPush(cg_arena, out, MD_S8Lit(" "));
                }
            }
        }break;
        
        case CG_TableOp_CheckIfTrue:
        {
            MD_i64 check_val = CG_TableExprEvaluate_Numeric(info, expr->left);
            if(check_val)
            {
                CG_TableExprEvaluate_String(info, expr->right, out);
            }
        }break;
        
        case CG_TableOp_Concat:
        {
            CG_TableExprEvaluate_String(info, expr->left, out);
            CG_TableExprEvaluate_String(info, expr->right, out);
        }break;
    }
}

static void
CG_LoopExpansionDimension(CG_ExpandIter *it, CG_ExpandInfo *info, MD_String8List *out)
{
    if(it->next)
    {
        for(MD_u64 idx = 0; idx < it->count; idx += 1)
        {
            it->idx = idx;
            CG_LoopExpansionDimension(it->next, info, out);
        }
    }
    else
    {
        for(MD_u64 idx = 0; idx < it->count; idx += 1)
        {
            it->idx = idx;
            MD_String8List expansion_strs = {0};
            MD_u64 start_idx = 0;
            for(MD_u64 char_idx = 0; char_idx <= info->strexpr.size; char_idx += 1)
            {
                MD_b32 is_expr_marker = info->strexpr.str[char_idx] == '$';
                
                // rjf: push regular string contents
                if(char_idx == info->strexpr.size || is_expr_marker)
                {
                    MD_String8 normal_string_chunk = MD_S8Substring(info->strexpr, start_idx, char_idx);
                    MD_String8 escaped = CG_EscapedFromString(cg_arena, normal_string_chunk);
                    MD_S8ListPush(cg_arena, &expansion_strs, escaped);
                }
                
                // rjf: handle expansion
                if(is_expr_marker)
                {
                    MD_String8 expr_string = MD_S8Skip(info->strexpr, char_idx+1);
                    {
                        MD_i64 paren_nest = 0;
                        for(MD_u64 expr_str_char_idx = 0; expr_str_char_idx < expr_string.size; expr_str_char_idx += 1)
                        {
                            if(expr_string.str[expr_str_char_idx] == '(')
                            {
                                paren_nest += 1;
                            }
                            else if(expr_string.str[expr_str_char_idx] == ')')
                            {
                                paren_nest -= 1;
                                if(paren_nest == 0)
                                {
                                    expr_string.size = expr_str_char_idx+1;
                                    break;
                                }
                            }
                        }
                    }
                    MD_ParseResult parse = MD_ParseOneNode(cg_arena, expr_string, 0);
                    MD_Node *node = parse.node;
                    MD_ExprParseResult expr_parse = MD_ExprParse(cg_arena, &info->expr_op_table, node->first_child, MD_NilNode());
                    MD_Expr *expr = expr_parse.expr;
                    CG_TableExprEvaluate_String(info, expr, &expansion_strs);
                    MD_String8 parsed_string = MD_S8Substring(info->strexpr, char_idx+1, char_idx+1+parse.string_advance);
                    parsed_string = MD_S8ChopWhitespace(parsed_string);
                    start_idx = char_idx+1+parsed_string.size;
                }
            }
            
            // rjf: push expansion string to output list
            MD_String8 expansion_str = MD_S8ListJoin(cg_arena, expansion_strs, 0);
            MD_S8ListPush(cg_arena, out, expansion_str);
        }
    }
}

static MD_String8List
CG_GenStringListFromNode(MD_ExprOprTable expr_op_table, MD_Node *gen)
{
    MD_String8List result = {0};
    MD_ArenaTemp scratch = MD_GetScratch(0, 0);
    
    for(MD_EachNode(strexpr, gen->first_child))
    {
        //- rjf: build expansion iterator list
        CG_ExpandIter *first_iter = 0;
        CG_ExpandIter *last_iter = 0;
        {
            for(MD_EachNode(tag, strexpr->first_tag))
            {
                if(MD_S8Match(tag->string, MD_S8Lit("expand"), 0))
                {
                    MD_Node *table_name_node = MD_ChildFromIndex(tag, 0);
                    MD_Node *label_node = MD_ChildFromIndex(tag, 1);
                    MD_String8 table_name = table_name_node->string;
                    MD_String8 label = label_node->string;
                    
                    // rjf: grab the table associated with table_name
                    CG_NodeGrid *grid = 0;
                    {
                        MD_MapSlot *slot = MD_MapLookup(&cg_tbl_top_level_node_grid_map, MD_MapKeyStr(table_name));
                        if(slot != 0)
                        {
                            grid = slot->val;
                        }
                    }
                    
                    // rjf: grab the table header associated with table_name
                    CG_TableHeader *header = 0;
                    {
                        MD_MapSlot *slot = MD_MapLookup(&cg_tbl_top_level_table_header_map, MD_MapKeyStr(table_name));
                        if(slot != 0)
                        {
                            header = slot->val;
                        }
                    }
                    
                    // rjf: make iterator node if we got a grid
                    if(grid != 0)
                    {
                        CG_ExpandIter *iter = MD_PushArrayZero(scratch.arena, CG_ExpandIter, 1);
                        MD_QueuePush(first_iter, last_iter, iter);
                        iter->grid   = grid;
                        iter->header = header;
                        iter->label  = label;
                        iter->count  = grid->row_parents.count;
                    }
                    
                    // rjf: print out an error if grid is 0
                    if(grid == 0)
                    {
                        MD_PrintMessageFmt(stderr, MD_CodeLocFromNode(tag), MD_MessageKind_Error, "Table \"%S\" was not found.", table_name);
                    }
                    
                }
            }
        }
        
        //- rjf: generate string list for this strexpr & push to result
        if(first_iter != 0)
        {
            CG_ExpandInfo info = {0};
            {
                info.strexpr = strexpr->string;
                info.first_expand_iter = first_iter;
                info.expr_op_table = expr_op_table;
            }
            CG_LoopExpansionDimension(first_iter, &info, &result);
        }
        //- rjf: generate non-expansion strings
        else
        {
            MD_String8 escaped = CG_EscapedFromString(cg_arena, strexpr->string);
            MD_S8ListPush(cg_arena, &result, escaped);
        }
        
    }
    
    MD_ReleaseScratch(scratch);
    return result;
}

static void
CG_TBL_Generate(MD_Node *file_list)
{
    //- rjf: initialize all maps
    cg_tbl_top_level_node_grid_map    = MD_MapMake(cg_arena);
    cg_tbl_top_level_table_header_map = MD_MapMake(cg_arena);
    cg_tbl_layer_map_gen              = MD_MapMake(cg_arena);
    cg_tbl_layer_map_gen_enum         = MD_MapMake(cg_arena);
    cg_tbl_layer_map_gen_data         = MD_MapMake(cg_arena);
    
    //- rjf: build table expression operator table
    MD_ExprOprTable table_expr_op_table = {0};
    {
        MD_ExprOprList ops_list = {0};
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 10, MD_S8Lit("."),  CG_TableOp_Dot, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Prefix, 9,  MD_S8Lit("=>"), CG_TableOp_Bump, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 6,  MD_S8Lit("??"), CG_TableOp_CheckIfTrue, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 7,  MD_S8Lit(".."), CG_TableOp_Concat, 0);
        
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 8,  MD_S8Lit("=="), CG_TableOp_Equal, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 8,  MD_S8Lit("!="), CG_TableOp_IsNotEqual, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 5,  MD_S8Lit("&&"), CG_TableOp_BooleanAnd, 0);
        MD_ExprOprPush(cg_arena, &ops_list, MD_ExprOprKind_Binary, 4,  MD_S8Lit("||"), CG_TableOp_BooleanOr, 0);
        table_expr_op_table = MD_ExprBakeOprTableFromList(cg_arena, &ops_list);
    }
    
    //- rjf: gather phase
    for(MD_EachNode(file_ref, file_list->first_child))
    {
        MD_Node *file = MD_ResolveNodeFromReference(file_ref);
        MD_String8 layer_name = file->string;
        MD_MapKey layer_key = MD_MapKeyStr(layer_name);
        for(MD_EachNode(node, file->first_child))
        {
            MD_Node *table_tag = MD_TagFromString(node, cg_tbl_tag__table, 0);
            if(!MD_NodeIsNil(table_tag))
            {
                CG_NodeGrid *grid = MD_PushArrayZero(cg_arena, CG_NodeGrid, 1);
                *grid = CG_GridFromNode(node);
                MD_MapOverwrite(cg_arena, &cg_tbl_top_level_node_grid_map, MD_MapKeyStr(node->string), grid);
                CG_TableHeader *header = MD_PushArrayZero(cg_arena, CG_TableHeader, 1);
                *header = CG_TableHeaderFromTag(table_tag);
                MD_MapOverwrite(cg_arena, &cg_tbl_top_level_table_header_map, MD_MapKeyStr(node->string), header);
            }
            if(MD_NodeHasTag(node, cg_tbl_tag__table_gen, 0))
            {
                MD_MapInsert(cg_arena, &cg_tbl_layer_map_gen, layer_key, node);
            }
            if(MD_NodeHasTag(node, cg_tbl_tag__table_gen_enum, 0))
            {
                MD_MapInsert(cg_arena, &cg_tbl_layer_map_gen_enum, layer_key, node);
            }
            if(MD_NodeHasTag(node, cg_tbl_tag__table_gen_data, 0))
            {
                MD_MapInsert(cg_arena, &cg_tbl_layer_map_gen_data, layer_key, node);
            }
        }
    }
    
    //- rjf: generation phase
    for(MD_EachNode(file_ref, file_list->first_child))
    {
        MD_Node *file = MD_ResolveNodeFromReference(file_ref);
        MD_String8 layer_name = file->string;
        MD_MapKey layer_key = MD_MapKeyStr(layer_name);
        
        //- rjf: generate all table enums
        for(MD_MapSlot *slot = MD_MapLookup(&cg_tbl_layer_map_gen_enum, layer_key);
            slot != 0;
            slot = MD_MapScan(slot->next, layer_key))
        {
            MD_Node *gen = (MD_Node *)slot->val;
            CG_FilePair f = CG_FilePairFromNode(gen);
            fprintf(f.h, "typedef enum %.*s\n{\n", MD_S8VArg(gen->string));
            MD_String8List gen_strings = CG_GenStringListFromNode(table_expr_op_table, gen);
            MD_StringJoin join = { MD_S8Lit(""), MD_S8Lit("\n"), MD_S8Lit("") };
            MD_String8 gen_string = MD_S8ListJoin(cg_arena, gen_strings, &join);
            fprintf(f.h, "%.*s", MD_S8VArg(gen_string));
            fprintf(f.h, "\n}\n%.*s;\n\n", MD_S8VArg(gen->string));
        }
        
        //- rjf: generate all data tables
        for(MD_MapSlot *slot = MD_MapLookup(&cg_tbl_layer_map_gen_data, layer_key);
            slot != 0;
            slot = MD_MapScan(slot->next, layer_key))
        {
            MD_Node *gen = (MD_Node *)slot->val;
            MD_Node *tag = MD_TagFromString(gen, cg_tbl_tag__table_gen_data, 0);
            MD_Node *data_table_type_node = tag->first_child;
            MD_String8 data_table_type = data_table_type_node->string;
            MD_String8List gen_strings = CG_GenStringListFromNode(table_expr_op_table, gen);
            MD_b32 is_engine = MD_NodeHasTag(gen, cg_tbl_tag__engine, MD_StringMatchFlag_CaseInsensitive);
            
            MD_String8 h_decl_specifier = MD_S8Lit("extern");
            if(is_engine)
            {
                h_decl_specifier = MD_S8Lit("engine_global");
            }
            
            CG_FilePair f = CG_FilePairFromNode(gen);
            fprintf(f.h, "%.*s %.*s %.*s[%" PRIu64 "];\n\n", MD_S8VArg(h_decl_specifier), MD_S8VArg(data_table_type), MD_S8VArg(gen->string), gen_strings.node_count);
            
            if(is_engine)
            {
                fprintf(f.c, "#if ENGINE\n");
            }
            fprintf(f.c, "%.*s %.*s[%" PRIu64 "] =\n{\n", MD_S8VArg(data_table_type), MD_S8VArg(gen->string), gen_strings.node_count);
            MD_StringJoin join = { MD_S8Lit(""), MD_S8Lit("\n"), MD_S8Lit("") };
            MD_String8 gen_string = MD_S8ListJoin(cg_arena, gen_strings, &join);
            fprintf(f.c, "%.*s", MD_S8VArg(gen_string));
            fprintf(f.c, "\n};\n");
            if(is_engine)
            {
                fprintf(f.c, "#endif // ENGINE\n");
            }
            fprintf(f.c, "\n");
        }
        
        //- rjf: generate all general generations
        for(MD_MapSlot *slot = MD_MapLookup(&cg_tbl_layer_map_gen, layer_key);
            slot != 0;
            slot = MD_MapScan(slot->next, layer_key))
        {
            MD_Node *gen = (MD_Node *)slot->val;
            CG_FilePair f = CG_FilePairFromNode(gen);
            FILE *file = MD_NodeHasTag(gen, MD_S8Lit("c"), 0) ? f.c : f.h;
            MD_String8List gen_strings = CG_GenStringListFromNode(table_expr_op_table, gen);
            MD_StringJoin join = { MD_S8Lit(""), MD_S8Lit("\n"), MD_S8Lit("") };
            MD_String8 gen_string = MD_S8ListJoin(cg_arena, gen_strings, &join);
            fprintf(file, "%.*s", MD_S8VArg(gen_string));
            fprintf(file, "\n\n");
        }
        
    }
    
}
