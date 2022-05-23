////////////////////////////////
//~ rjf: Functions

engine_function U64
SP_PieceMatchAdvanceFromString(String8 string, SP_Piece *piece)
{
    U64 advance = 0;
    if(string.size != 0 && piece != 0)
    {
        for(SP_Atom *atom = piece->first; atom != 0; atom = atom->next)
        {
            // rjf: allow everything
            if(atom->allow_all)
            {
                advance = 1;
                break;
            }
            
            // rjf: simple string requirement
            else if(atom->string.size != 0)
            {
                if(Str8Match(Substr8(string, R1U64(0, atom->string.size)), atom->string, 0))
                {
                    advance = atom->string.size;
                    break;
                }
            }
            
            // rjf: codepoint range
            else if(atom->min_codepoint != 0)
            {
                if(atom->min_codepoint <= string.str[0] && string.str[0] <= atom->max_codepoint)
                {
                    advance = 1;
                    break;
                }
            }
        }
    }
    return advance;
}

engine_function U64
SP_PatternMatchAdvanceFromString(String8 string, SP_Pattern pattern)
{
    U64 result = 0;
    if(string.size != 0)
    {
        // rjf: check against accelerator first
        B32 good_accelerator_check = 0;
        if(pattern.first_byte_accelerator[string.str[0]/64] & (1ull<<(string.str[0]%64)))
        {
            good_accelerator_check = 1;
        }
        
        // rjf: do complex piece matching
        if(good_accelerator_check)
        {
            SP_Piece *piece = pattern.first_piece;
            SP_Piece *continuation_piece = 0;
            for(U64 idx = 0; idx < string.size && (piece != 0 || continuation_piece != 0);)
            {
                U64 last_advance = SP_PieceMatchAdvanceFromString(Str8Skip(string, idx), continuation_piece);
                U64 this_advance = SP_PieceMatchAdvanceFromString(Str8Skip(string, idx), piece);
                if(this_advance != 0)
                {
                    continuation_piece = 0;
                    idx += this_advance;
                    result = idx;
                    if(piece->flags & SP_PieceFlag_AllowMany)
                    {
                        continuation_piece = piece;
                    }
                    piece = piece->next;
                }
                else if(last_advance != 0)
                {
                    idx += last_advance;
                    result = idx;
                }
                else if(piece != 0 && piece->flags & SP_PieceFlag_Optional)
                {
                    piece = piece->next;
                }
                else
                {
                    break;
                }
            }
            for(SP_Piece *p = piece; p != 0; p = p->next)
            {
                if(!(p->flags & SP_PieceFlag_Optional))
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}

engine_function void
SP_PatternListPush(M_Arena *arena, SP_PatternList *list, SP_Pattern pattern)
{
    SP_PatternNode *n = PushArrayZero(arena, SP_PatternNode, 1);
    n->pattern = pattern;
    QueuePush(list->first, list->last, n);
    list->count += 1;
}
