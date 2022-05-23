engine_function TM_Theme
TM_ThemeMake(M_Arena *arena)
{
    TM_Theme theme = {0};
    theme.table_size = 256;
    theme.table = PushArrayZero(arena, TM_ThemeColorNode *, theme.table_size);
    return theme;
}

engine_function U64
TM_HashFromString(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + CharToLower(string.str[i]);
    }
    return result;
}

engine_function void
TM_ThemeSetColor(M_Arena *arena, TM_Theme *theme, String8 string, Vec4F32 color)
{
    TM_ThemeColorNode *node = 0;
    U64 hash = TM_HashFromString(string);
    U64 slot = hash%theme->table_size;
    for(TM_ThemeColorNode *n = theme->table[slot]; n != 0; n = n->hash_next)
    {
        if(Str8Match(n->string, string, MatchFlag_CaseInsensitive))
        {
            node = n;
            break;
        }
    }
    if(node == 0)
    {
        node = PushArrayZero(arena, TM_ThemeColorNode, 1);
        node->string = PushStr8Copy(arena, string);
        node->hash_next = theme->table[slot];
        theme->table[slot] = node;
    }
    node->color = color;
}

engine_function TM_ThemeColorNode *
TM_ThemeColorNodeFromString(TM_Theme *theme, String8 string)
{
    TM_ThemeColorNode *result = 0;
    U64 hash = TM_HashFromString(string);
    U64 slot = hash%theme->table_size;
    for(TM_ThemeColorNode *n = theme->table[slot]; n != 0; n = n->hash_next)
    {
        if(Str8Match(n->string, string, MatchFlag_CaseInsensitive))
        {
            result = n;
            break;
        }
    }
    return result;
}

engine_function Vec4F32
TM_ThemeColorFromString(TM_Theme *theme, String8 string)
{
    Vec4F32 result = {1, 0, 1, 1};
    TM_ThemeColorNode *node = TM_ThemeColorNodeFromString(theme, string);
    if(node != 0)
    {
        result = node->color;
    }
    return result;
}
