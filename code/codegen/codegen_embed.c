static void
CG_EMBED_Generate(MD_Node *file_list)
{
    for(MD_EachNode(file_ref, file_list->first_child))
    {
        MD_Node *file = MD_ResolveNodeFromReference(file_ref);
        for(MD_EachNode(node, file->first_child))
        {
            CG_FilePair f = CG_FilePairFromNode(node);
            if(MD_NodeHasTag(node, MD_S8Lit("embed"), MD_StringMatchFlag_CaseInsensitive))
            {
                fprintf(f.h, "read_only global String8 %.*s =\nStr8LitComp(", MD_S8VArg(node->string));
                CG_GenerateMultilineStringAsCLiteral(f.h, node->first_child->string);
                fprintf(f.h, ");\n\n");
            }
        }
    }
}
