////////////////////////////////
//~ rjf: Globals

#if ENGINE
VIN_Node vin_g_nil_node =
{
    &vin_g_nil_node,
    {0},
};
VIN_State *vin_state = 0;
#endif

////////////////////////////////
//~ rjf: Functions

engine_function void
VIN_Init(void)
{
    M_Arena *arena = M_ArenaAlloc(Gigabytes(4));
    vin_state = PushArrayZero(arena, VIN_State, 1);
    vin_state->arena = arena;
    vin_state->table_size = 4096;
    vin_state->table = PushArrayZero(arena, VIN_Node *, vin_state->table_size);
}

engine_function U64
VIN_HashFromString(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}

engine_function VIN_Value *
VIN_ValueFromString(VIN_Literal default_value, String8 string)
{
    U64 hash = VIN_HashFromString(string);
    U64 slot = hash % vin_state->table_size;
    VIN_Node *node = &vin_g_nil_node;
    for(VIN_Node *n = vin_state->table[slot]; n != 0; n = n->next)
    {
        if(Str8Match(string, n->key, 0))
        {
            node = n;
            break;
        }
    }
    if(node == &vin_g_nil_node)
    {
        node = PushArrayZero(vin_state->arena, VIN_Node, 1);
        node->key = PushStr8Copy(vin_state->arena, string);
        MemoryCopy(node->value.storage, &default_value.u64, sizeof(U64));
        node->next = vin_state->table[slot];
        vin_state->table[slot] = node;
    }
    VIN_Value *value = &node->value;
    return value;
}

engine_function VIN_Value *
VIN_ValueFromStringF(VIN_Literal default_value, char *fmt, ...)
{
    M_Temp scratch = GetScratch(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = PushStr8FV(scratch.arena, fmt, args);
    va_end(args);
    VIN_Value *result = VIN_ValueFromString(default_value, string);
    ReleaseScratch(scratch);
    return result;
}
