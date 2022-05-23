engine_function String8
Str8FromMD(MD_String8 str)
{
    String8 result = { str.str, str.size };
    return result;
}

engine_function MD_String8
MDFromStr8(String8 str)
{
    MD_String8 result = { str.str, str.size };
    return result;
}

engine_function SP_Pattern
SP_PatternFromMDNode(M_Arena *arena, MD_Node *node)
{
    SP_Pattern pattern = {0};
    
    //- rjf: parse out pieces
    for(MD_Node *md_piece = node->first_child;
        !MD_NodeIsNil(md_piece);)
    {
        SP_Piece *piece = PushArrayZero(arena, SP_Piece, 1);
        for(MD_Node *md_atom = md_piece->first_child;
            !MD_NodeIsNil(md_atom);)
        {
            SP_Atom *atom = PushArrayZero(arena, SP_Atom, 1);
            
            // rjf: range of characters
            if(MD_S8Match(md_atom->next->string, MD_S8Lit(".."), 0) &&
               md_atom->string.size != 0 &&
               md_atom->next->next->string.size != 0)
            {
                atom->min_codepoint = md_atom->string.str[0];
                atom->max_codepoint = md_atom->next->next->string.str[0];
                md_atom = md_atom->next->next->next;
            }
            
            // rjf: allow all
            else if(md_atom->flags & MD_NodeFlag_Symbol && MD_S8Match(md_atom->string, MD_S8Lit("~"), 0))
            {
                atom->allow_all = 1;
                md_atom = md_atom->next;
            }
            
            // rjf: simple string requirement
            else if(md_atom->string.size != 0)
            {
                atom->string = PushStr8Copy(arena, Str8FromMD(md_atom->string));
                md_atom = md_atom->next;
            }
            
            // rjf: busted atom
            else
            {
                md_atom = md_atom->next;
            }
            
            QueuePush(piece->first, piece->last, atom);
            piece->atom_count += 1;
        }
        
        // rjf: collect flags & advance
        if(MD_S8Match(md_piece->next->string, MD_S8Lit("*"), 0))
        {
            piece->flags |= SP_PieceFlag_AllowMany;
            md_piece = md_piece->next->next;
        }
        else if(MD_S8Match(md_piece->next->string, MD_S8Lit("?"), 0))
        {
            piece->flags |= SP_PieceFlag_Optional;
            md_piece = md_piece->next->next;
        }
        else if(MD_S8Match(md_piece->next->string, MD_S8Lit("*?"), 0) ||
                MD_S8Match(md_piece->next->string, MD_S8Lit("?*"), 0))
        {
            piece->flags |= SP_PieceFlag_Optional|SP_PieceFlag_AllowMany;
            md_piece = md_piece->next->next;
        }
        else
        {
            md_piece = md_piece->next;
        }
        
        QueuePush(pattern.first_piece, pattern.last_piece, piece);
        pattern.piece_count += 1;
    }
    
    //- rjf: build first-byte accelerator
    for(SP_Piece *piece = pattern.first_piece; piece != 0; piece = piece->next)
    {
        for(SP_Atom *atom = piece->first; atom != 0; atom = atom->next)
        {
            if(atom->allow_all)
            {
                pattern.first_byte_accelerator[0] = 0xffffffffffffffff;
                pattern.first_byte_accelerator[1] = 0xffffffffffffffff;
                pattern.first_byte_accelerator[2] = 0xffffffffffffffff;
                pattern.first_byte_accelerator[3] = 0xffffffffffffffff;
            }
            else if(atom->string.size != 0)
            {
                U64 byte = atom->string.str[0];
                pattern.first_byte_accelerator[byte/64] |= (1ull << (byte%64));
            }
            else if(atom->min_codepoint != 0)
            {
                for(U32 byte = atom->min_codepoint; byte <= atom->max_codepoint && 0 <= byte && byte <= 255; byte += 1)
                {
                    pattern.first_byte_accelerator[byte/64] |= (1ull << (byte%64));
                }
            }
        }
        if(!(piece->flags & SP_PieceFlag_Optional))
        {
            break;
        }
    }
    
    return pattern;
}
