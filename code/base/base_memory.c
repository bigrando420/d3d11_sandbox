#if !defined(M_IMPL_Reserve)
#error M_IMPL_Reserve must be defined to use base memory.
#endif
#if !defined(M_IMPL_Release)
#error M_IMPL_Release must be defined to use base memory.
#endif
#if !defined(M_IMPL_Commit)
#error M_IMPL_Commit must be defined to use base memory.
#endif
#if !defined(M_IMPL_Decommit)
#error M_IMPL_Decommit must be defined to use base memory.
#endif

////////////////////////////////
//~ rjf: Globals

#if ENGINE
read_only M_Node m_g_nil_node =
{
    &m_g_nil_node,
    &m_g_nil_node,
    &m_g_nil_node,
    &m_g_nil_node,
    &m_g_nil_node,
};
#endif

////////////////////////////////
//~ rjf: Base Node Functions

engine_function M_Node *
M_NilNode(void)
{
    return &m_g_nil_node;
}

engine_function B32
M_NodeIsNil(M_Node *node)
{
    return node == 0 || node == &m_g_nil_node;
}

engine_function M_Node *
M_NodeAlloc(U64 cap)
{
    M_Node *node = M_IMPL_Reserve(cap);
    M_IMPL_Commit(node, M_COMMIT_SIZE);
    node->first = node->last = node->next = node->prev = node->parent = M_NilNode();
    node->size = cap;
    return node;
}

engine_function void
M_NodeRelease(M_Node *node)
{
    for(M_Node *child = node->first, *next = 0; !M_NodeIsNil(child); child = next)
    {
        next = child->next;
        M_NodeRelease(child);
    }
    if(M_NodeIsNil(node->parent))
    {
        M_NodeRemoveChild(node->parent, node);
    }
    M_IMPL_Release(node);
}

engine_function void
M_NodeInsertChild(M_Node *parent, M_Node *prev_child, M_Node *new_child)
{
    DLLInsert_NPZ(parent->first, parent->last, prev_child, new_child, next, prev, M_CheckNilNode, M_SetNilNode);
    new_child->parent = parent;
}

engine_function void
M_NodeRemoveChild(M_Node *parent, M_Node *node)
{
    DLLRemove_NPZ(parent->first, parent->last, node, next, prev, M_CheckNilNode, M_SetNilNode);
    node->parent = M_NilNode();
}

////////////////////////////////
//~ rjf: Arena Functions

engine_function M_Arena *
M_ArenaAlloc(U64 cap)
{
    M_Node *node = M_NodeAlloc(cap);
    M_Arena *arena = (M_Arena *)node;
    arena->commit_pos = M_COMMIT_SIZE;
    arena->pos = sizeof(M_Arena);
    arena->align = 8; 
    return arena;
}

engine_function M_Arena *
M_ArenaAllocDefault(void)
{
    return M_ArenaAlloc(Gigabytes(4));
}

engine_function void
M_ArenaRelease(M_Arena *arena)
{
    M_NodeRelease(&arena->node);
}

engine_function void *
M_ArenaPushAligned(M_Arena *arena, U64 size, U64 align)
{
    void *memory = 0;
    align = ClampBot(arena->align, align);
    
    U64 pos = arena->pos;
    U64 pos_address = IntFromPtr(arena) + pos;
    U64 aligned_address = pos_address + align - 1;
    aligned_address -= aligned_address%align;
    
    U64 alignment_size = aligned_address - pos_address;
    if (pos + alignment_size + size <= arena->node.size)
    {
        U8 *mem_base = (U8*)arena;
        memory = mem_base + pos + alignment_size;
        U64 new_pos = pos + alignment_size + size;
        arena->pos = new_pos;
        
        if (new_pos > arena->commit_pos)
        {
            U64 commit_grow = new_pos - arena->commit_pos;
            commit_grow += M_COMMIT_SIZE - 1;
            commit_grow -= commit_grow%M_COMMIT_SIZE;
            M_IMPL_Commit(mem_base + arena->commit_pos, commit_grow);
            arena->commit_pos += commit_grow;
        }
    }
    
    return memory;
}

engine_function void *
M_ArenaPush(M_Arena *arena, U64 size)
{
    return M_ArenaPushAligned(arena, size, arena->align);
}

engine_function void *
M_ArenaPushZero(M_Arena *arena, U64 size)
{
    void *memory = M_ArenaPush(arena, size);
    MemoryZero(memory, size);
    return memory;
}

engine_function void
M_ArenaSetPosBack(M_Arena *arena, U64 pos)
{
    pos = Max(pos, sizeof(*arena));
    if(arena->pos > pos)
    {
        arena->pos = pos;
        
        U64 decommit_pos = pos;
        decommit_pos += M_COMMIT_SIZE - 1;
        decommit_pos -= decommit_pos%M_COMMIT_SIZE;
        U64 over_committed = arena->commit_pos - decommit_pos;
        over_committed -= over_committed%M_COMMIT_SIZE;
        if(decommit_pos > 0 && over_committed >= M_DECOMMIT_THRESHOLD)
        {
            M_IMPL_Decommit((U8*)arena + decommit_pos, over_committed);
            arena->commit_pos -= over_committed;
        }
    }
}

engine_function void
M_ArenaSetAutoAlign(M_Arena *arena, U64 align)
{
    arena->align = align;
}

engine_function void
M_ArenaPop(M_Arena *arena, U64 size)
{
    M_ArenaSetPosBack(arena, arena->pos-size);
}

engine_function void
M_ArenaClear(M_Arena *arena)
{
    M_ArenaPop(arena, arena->pos);
}

engine_function U64
M_ArenaGetPos(M_Arena *arena)
{
    return arena->pos;
}

////////////////////////////////
//~ rjf: Temp

engine_function M_Temp
M_BeginTemp(M_Arena *arena)
{
    M_Temp result = {arena, arena->pos};
    return result;
}

engine_function void
M_EndTemp(M_Temp temp)
{
    M_ArenaSetPosBack(temp.arena, temp.pos);
}

////////////////////////////////
//~ rjf: String Heap Functions

engine_function M_Heap *
M_HeapAlloc(U64 cap)
{
    M_Arena *arena = M_ArenaAlloc(cap);
    M_Heap *heap = PushArrayZero(arena, M_Heap, 1);
    heap->arena = arena;
    heap->slot_table_size = 64;
    heap->slot_table = PushArrayZero(arena, M_HeapSlot, heap->slot_table_size);
    return heap;
}

engine_function void
M_HeapRelease(M_Heap *heap)
{
    M_ArenaRelease(heap->arena);
}

engine_function void *
M_HeapAllocData(M_Heap *heap, U64 size)
{
    U64 needed_size = size + sizeof(M_HeapChunk);
    U64 next_pow2 = UpToPow2_64(needed_size);
    U64 slot_index = SearchFirstOneBit_64_BinarySearch(next_pow2);
    M_HeapSlot *slot = &heap->slot_table[slot_index];
    M_HeapChunk *chunk = slot->first_free;
    if(chunk == 0)
    {
        chunk = (M_HeapChunk *)M_ArenaPush(heap->arena, next_pow2);
        chunk->size = next_pow2;
    }
    else
    {
        slot->first_free = slot->first_free->next;
    }
    return (void *)(chunk + 1);
}

engine_function void
M_HeapReleaseData(M_Heap *heap, void *ptr)
{
    if(ptr != 0)
    {
        M_HeapChunk *chunk = (M_HeapChunk *)ptr - 1;
        U64 size = chunk->size;
        U64 slot_index = SearchFirstOneBit_64_BinarySearch(size);
        chunk->next = heap->slot_table[slot_index].first_free;
        heap->slot_table[slot_index].first_free = chunk;
    }
}

engine_function void *
M_HeapReAllocData(M_Heap *heap, void *ptr, U64 new_size)
{
    void *result = 0;
    if(ptr == 0)
    {
        result = M_HeapAllocData(heap, new_size);
    }
    else
    {
        M_HeapChunk *chunk = (M_HeapChunk *)ptr - 1;
        if(chunk->size >= new_size+sizeof(M_HeapChunk))
        {
            result = ptr;
        }
        else
        {
            M_HeapReleaseData(heap, ptr);
            ptr = M_HeapAllocData(heap, new_size);
        }
    }
    return result;
}
