static MD_String8
CG_ENUM_ChildMapExprStr(MD_Node *by, MD_Node *node)
{
    MD_String8List list = {0};
    
    MD_u64 start = 0;
    MD_b32 child_found = 0;
    for(MD_u64 i = 0; i <= by->string.size; i += 1)
    {
        if(i == by->string.size || by->string.str[i] == '$')
        {
            MD_String8 substr = MD_S8Substring(by->string, start, i);
            MD_S8ListPush(cg_arena, &list, substr);
            start = i;
        }
        if(i < by->string.size && by->string.str[i] == '$')
        {
            MD_Token token = MD_TokenFromString(MD_S8Skip(by->string, i+1));
            if(token.kind & MD_TokenKind_Identifier)
            {
                MD_Node *child_map = MD_ChildFromString(node, token.string, MD_StringMatchFlag_CaseInsensitive);
                for(MD_EachNode(map_child, child_map->first_child))
                {
                    MD_S8ListPush(cg_arena, &list, map_child->string);
                }
                if(!MD_NodeIsNil(child_map))
                {
                    child_found = 1;
                }
            }
            else if(token.kind & MD_TokenKind_Numeric)
            {
                MD_Node *child_map = MD_ChildFromIndex(node, MD_CStyleIntFromString(token.string));
                MD_S8ListPush(cg_arena, &list, child_map->string);
                if(!MD_NodeIsNil(child_map))
                {
                    child_found = 1;
                }
            }
            start = i + token.raw_string.size + 1;
        }
    }
    
    MD_String8 result = child_found ? MD_S8ListJoin(cg_arena, list, 0) : MD_S8Lit("");
    return result;
}

static void
CG_ENUM_Generate(MD_Node *file_list)
{
    for(MD_EachNode(file_ref, file_list->first_child))
    {
        MD_Node *file = MD_ResolveNodeFromReference(file_ref);
        for(MD_EachNode(node, file->first_child))
        {
            CG_FilePair f = CG_FilePairFromNode(node);
            MD_b32 is_enum = MD_NodeHasTag(node, MD_S8Lit("enum"), MD_StringMatchFlag_CaseInsensitive);
            
            //- rjf: generate enum definition
            if(is_enum)
            {
                fprintf(f.h, "typedef enum %.*s\n{\n", MD_S8VArg(node->string));
                for(MD_EachNode(child, node->first_child))
                {
                    fprintf(f.h, "%.*s_%.*s,\n", MD_S8VArg(node->string), MD_S8VArg(child->string));
                }
                fprintf(f.h, "%.*s_COUNT,\n", MD_S8VArg(node->string));
                fprintf(f.h, "}\n%.*s;\n", MD_S8VArg(node->string));
            }
            
            //- rjf: generate enum maps
            if(is_enum)
            {
                for(MD_EachNode(tag, node->first_tag))
                {
                    if(MD_S8Match(tag->string, MD_S8Lit("map"), MD_StringMatchFlag_CaseInsensitive))
                    {
                        //- rjf: parse
                        MD_Node *map_name_node = MD_ChildFromIndex(tag, 0);
                        MD_Node *map_expr_first = map_name_node->next;
                        MD_Node *map_type_dst = MD_NilNode();
                        MD_Node *map_by_expr = MD_NilNode();
                        MD_Node *map_default_expr = MD_NilNode();
                        for(MD_EachNode(spec, map_expr_first))
                        {
                            if(MD_S8Match(spec->string, MD_S8Lit("->"), 0))
                            {
                                map_type_dst = spec->next;
                            }
                            if(MD_S8Match(spec->string, MD_S8Lit("by"), MD_StringMatchFlag_CaseInsensitive))
                            {
                                map_by_expr = spec->next;
                            }
                            if(MD_S8Match(spec->string, MD_S8Lit("default"), MD_StringMatchFlag_CaseInsensitive))
                            {
                                map_default_expr = spec->next;
                            }
                        }
                        
                        //- rjf: generate map
                        fprintf(f.c, "function %.*s\n%.*s(%.*s v)\n{\n",
                                MD_S8VArg(map_type_dst->string),
                                MD_S8VArg(map_name_node->string),
                                MD_S8VArg(node->string));
                        fprintf(f.c, "%.*s result = %.*s;\n", MD_S8VArg(map_type_dst->string), MD_S8VArg(map_default_expr->string));
                        fprintf(f.c, "switch(v)\n{\n");
                        fprintf(f.c, "default:break;\n");
                        for(MD_EachNode(child, node->first_child))
                        {
                            MD_String8 map_expr_str = CG_ENUM_ChildMapExprStr(map_by_expr, child);
                            if(map_expr_str.size != 0)
                            {
                                fprintf(f.c, "case %.*s_%.*s:{result = %.*s;}break;\n", MD_S8VArg(node->string), MD_S8VArg(child->string), MD_S8VArg(map_expr_str));
                            }
                        }
                        fprintf(f.c, "}\n");
                        fprintf(f.c, "return result;\n");
                        fprintf(f.c, "}\n\n");
                    }
                    
                    if(MD_S8Match(tag->string, MD_S8Lit("table"), MD_StringMatchFlag_CaseInsensitive))
                    {
                        MD_Node *table_name = MD_ChildFromIndex(tag, 0);
                        MD_Node *table_type = MD_ChildFromIndex(tag, 1);
                        MD_Node *table_element_expr = MD_ChildFromIndex(tag, 2);
                        MD_Node *table_default_expr = MD_ChildFromIndex(tag, 3);
                        
                        //- rjf: generate table
                        fprintf(f.c, "%.*s %.*s[] =\n{\n",
                                MD_S8VArg(table_type->string),
                                MD_S8VArg(table_name->string));
                        for(MD_EachNode(child, node->first_child))
                        {
                            MD_String8 map_expr_str = CG_ENUM_ChildMapExprStr(table_element_expr, child);
                            if(map_expr_str.size == 0)
                            {
                                map_expr_str = table_default_expr->string;
                            }
                            fprintf(f.c, "%.*s,\n", MD_S8VArg(map_expr_str));
                        }
                        fprintf(f.c, "};\n\n");
                    }
                }
            }
            
        }
    }
}
