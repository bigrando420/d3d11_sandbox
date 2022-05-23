#include "third_party/pcg/include/pcg_impl.c"

////////////////////////////////
//~ rjf: Bit Patterns

engine_function U32
UpToPow2_32(U32 x)
{
    if(x == 0)
    {
        x = 1;
    }
    else
    {
        x -= 1;
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        x += 1;
    }
    return x;
}

engine_function U64
UpToPow2_64(U64 x)
{
    if(x == 0)
    {
        x = 1;
    }
    else
    {
        x -= 1;
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        x |= (x >> 32);
        x += 1;
    }
    return x;
}

engine_function U32
SearchFirstOneBit_32_BinarySearch(U32 x)
{
    U32 result = 0;
    U32 t5 = x & 0xFFFF;
    if (!t5)
    {
        result += 16;
        x >>= 16;
    }
    U32 t4 = x & 0xFF;
    if (!t4)
    {
        result += 8;
        x >>= 8;
    }
    U32 t3 = x & 0xF;
    if (!t3)
    {
        result += 4;
        x >>= 4;
    }
    U32 t2 = x & 0x3;
    if (!t2)
    {
        result += 2;
        x >>= 2;
    }
    U32 t1 = x & 0x1;
    if (!t1)
    {
        result += 1;
    }
    if (x == 0)
    {
        result += 1;
    }
    return result;
}

engine_function U32
SearchFirstOneBit_64_BinarySearch(U64 x)
{
    U32 result = 0;
    U64 t6 = x & 0xFFFFFFFF;
    if (!t6)
    {
        result += 32;
        x >>= 32;
    }
    U64 t5 = x & 0xFFFF;
    if (!t5)
    {
        result += 16;
        x >>= 16;
    }
    U64 t4 = x & 0xFF;
    if (!t4)
    {
        result += 8;
        x >>= 8;
    }
    U64 t3 = x & 0xF;
    if (!t3)
    {
        result += 4;
        x >>= 4;
    }
    U64 t2 = x & 0x3;
    if (!t2)
    {
        result += 2;
        x >>= 2;
    }
    U64 t1 = x & 0x1;
    if (!t1)
    {
        result += 1;
    }
    if (x == 0)
    {
        result += 1;
    }
    return result;
}

////////////////////////////////
//~ rjf: Comparisons

engine_function B32
Compare_U64(U64 a, U64 b, Comparison comparison)
{
    B32 result = 0;
    switch(comparison)
    {
        default: break;
        case Comparison_EqualTo:               result = (a == b); break;
        case Comparison_NotEqualTo:            result = (a != b); break;
        case Comparison_LessThan:              result = (a <  b); break;
        case Comparison_LessThanOrEqualTo:     result = (a <= b); break;
        case Comparison_GreaterThan:           result = (a >  b); break;
        case Comparison_GreaterThanOrEqualTo:  result = (a >= b); break;
    }
    return result;
}

////////////////////////////////
//~ rjf: Random Number Generation Functions

engine_function Rng
MakeRng(U64 seed1, U64 seed2)
{
    Rng result = {0};
    pcg32_srandom_r(&result.rng, seed1, seed2);
    return result;
}
