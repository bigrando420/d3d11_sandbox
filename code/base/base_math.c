////////////////////////////////
//~ rjf: Vector Ops

engine_function Vec2F32
V2F32(F32 x, F32 y)
{
    Vec2F32 result = { x, y };
    return result;
}
engine_function Vec2F32 Add2F32(Vec2F32 a, Vec2F32 b) { return V2F32(a.x+b.x, a.y+b.y); }
engine_function Vec2F32 Sub2F32(Vec2F32 a, Vec2F32 b) { return V2F32(a.x-b.x, a.y-b.y); }
engine_function Vec2F32 Mul2F32(Vec2F32 a, Vec2F32 b) { return V2F32(a.x*b.x, a.y*b.y); }
engine_function Vec2F32 Div2F32(Vec2F32 a, Vec2F32 b) { return V2F32(a.x/b.x, a.y/b.y); }
engine_function Vec2F32 Scale2F32(Vec2F32 a, F32 scale) { return V2F32(a.x*scale, a.y*scale); }
engine_function F32 Dot2F32(Vec2F32 a, Vec2F32 b) { return (a.x*b.x + a.y*b.y); }
engine_function F32 LengthSquared2F32(Vec2F32 v) { return Dot2F32(v, v); }
engine_function F32 Length2F32(Vec2F32 v) { return SquareRoot(LengthSquared2F32(v)); }
engine_function Vec2F32 Normalize2F32(Vec2F32 v) { return Scale2F32(v, 1.f/Length2F32(v)); }
engine_function Vec2F32 Mix2F32(Vec2F32 a, Vec2F32 b, F32 t) { return V2F32(a.x*(1-t) + b.x*t, a.y*(1-t) + b.y*t); }

engine_function Vec2S32
V2S32(S32 x, S32 y)
{
    Vec2S32 result = { x, y };
    return result;
}

engine_function Vec3F32
V3F32(F32 x, F32 y, F32 z)
{
    Vec3F32 result = { x, y, z };
    return result;
}
engine_function Vec3F32 Add3F32(Vec3F32 a, Vec3F32 b) { return V3F32(a.x+b.x, a.y+b.y, a.z+b.z); }
engine_function Vec3F32 Sub3F32(Vec3F32 a, Vec3F32 b) { return V3F32(a.x-b.x, a.y-b.y, a.z-b.z); }
engine_function Vec3F32 Mul3F32(Vec3F32 a, Vec3F32 b) { return V3F32(a.x*b.x, a.y*b.y, a.z*b.z); }
engine_function Vec3F32 Div3F32(Vec3F32 a, Vec3F32 b) { return V3F32(a.x/b.x, a.y/b.y, a.z/b.z); }
engine_function Vec3F32 Scale3F32(Vec3F32 a, F32 scale) { return V3F32(a.x*scale, a.y*scale, a.z*scale); }
engine_function F32 Dot3F32(Vec3F32 a, Vec3F32 b) { return (a.x*b.x + a.y*b.y + a.z*b.z); }
engine_function F32 LengthSquared3F32(Vec3F32 v) { return Dot3F32(v, v); }
engine_function F32 Length3F32(Vec3F32 v) { return SquareRoot(LengthSquared3F32(v)); }
engine_function Vec3F32 Normalize3F32(Vec3F32 v) { return Scale3F32(v, 1.f/Length3F32(v)); }
engine_function Vec3F32 Mix3F32(Vec3F32 a, Vec3F32 b, F32 t) { return V3F32(a.x*(1-t) + b.x*t, a.y*(1-t) + b.y*t, a.z*(1-t) + b.z*t); }
engine_function Vec3F32 Cross3F32(Vec3F32 a, Vec3F32 b) { return V3F32(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }

engine_function Vec4F32
V4F32(F32 x, F32 y, F32 z, F32 w)
{
    Vec4F32 result = { x, y, z, w };
    return result;
}
engine_function Vec4F32 Add4F32(Vec4F32 a, Vec4F32 b) { return V4F32(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
engine_function Vec4F32 Sub4F32(Vec4F32 a, Vec4F32 b) { return V4F32(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }
engine_function Vec4F32 Mul4F32(Vec4F32 a, Vec4F32 b) { return V4F32(a.x*b.x, a.y*b.y, a.z*b.z, a.z*b.z); }
engine_function Vec4F32 Div4F32(Vec4F32 a, Vec4F32 b) { return V4F32(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w); }
engine_function Vec4F32 Scale4F32(Vec4F32 a, F32 scale) { return V4F32(a.x*scale, a.y*scale, a.z*scale, a.w*scale); }
engine_function F32 Dot4F32(Vec4F32 a, Vec4F32 b) { return (a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w); }
engine_function F32 LengthSquared4F32(Vec4F32 v) { return Dot4F32(v, v); }
engine_function F32 Length4F32(Vec4F32 v) { return SquareRoot(LengthSquared4F32(v)); }
engine_function Vec4F32 Normalize4F32(Vec4F32 v) { return Scale4F32(v, 1.f/Length4F32(v)); }
engine_function Vec4F32 Mix4F32(Vec4F32 a, Vec4F32 b, F32 t) { return V4F32(a.x*(1-t) + b.x*t, a.y*(1-t) + b.y*t, a.z*(1-t) + b.z*t, a.w*(1-t) + b.w*t); }
engine_function Vec4F32
Transform4F32(Vec4F32 v, Matrix4x4F32 m)
{
    Vec4F32 result = {0};
    for(int i = 0; i < 4; i += 1)
    {
        result.elements[i] = (v.elements[0]*m.elements[0][i] +
                              v.elements[1]*m.elements[1][i] +
                              v.elements[2]*m.elements[2][i] +
                              v.elements[3]*m.elements[3][i]);
    }
    return result;
}

engine_function Vec2S64
V2S64(S64 x, S64 y)
{
    Vec2S64 v;
    v.x = x;
    v.y = y;
    return v;
}

engine_function Vec2S64 Add2S64(Vec2S64 a, Vec2S64 b) { return V2S64(a.x+b.x, a.y+b.y); }
engine_function Vec2S64 Sub2S64(Vec2S64 a, Vec2S64 b) { return V2S64(a.x-b.x, a.y-b.y); }

////////////////////////////////
//~ rjf: Range Ops

engine_function Rng1F32 R1F32(F32 min, F32 max)
{
    Rng1F32 result = { min, max };
    if(result.max < result.min)
    {
        Swap(F32, result.min, result.max);
    }
    return result;
}
engine_function Rng1F32 Pad1F32(Rng1F32 r, F32 x) { return R1F32(r.min-x, r.max+x); }
engine_function F32 Center1F32(Rng1F32 r) { return (r.min + r.max)/2; }
engine_function B32 Contains1F32(Rng1F32 r, F32 v) { return r.min <= v && v < r.max; }
engine_function F32 Dim1F32(Rng1F32 r) { return AbsoluteValueF32(r.max - r.min); }
engine_function Rng1F32 Union1F32(Rng1F32 a, Rng1F32 b) { return R1F32(Min(a.min, b.min), Max(a.max, b.max)); }
engine_function Rng1F32 Intersection1F32(Rng1F32 a, Rng1F32 b) { return R1F32(Max(a.min, b.min), Min(a.max, b.max)); }

engine_function Rng1S64 R1S64(S64 min, S64 max)
{
    Rng1S64 result = { min, max };
    if(result.max < result.min)
    {
        Swap(S64, result.min, result.max);
    }
    return result;
}
engine_function Rng1S64 Pad1S64(Rng1S64 r, S64 x) { return R1S64(r.min-x, r.max+x); }
engine_function S64 Center1S64(Rng1S64 r) { return (r.min + r.max)/2; }
engine_function B32 Contains1S64(Rng1S64 r, S64 v) { return r.min <= v && v < r.max; }
engine_function S64 Dim1S64(Rng1S64 r) { return AbsoluteValueS64(r.max - r.min); }
engine_function Rng1S64 Union1S64(Rng1S64 a, Rng1S64 b) { return R1S64(Min(a.min, b.min), Max(a.max, b.max)); }
engine_function Rng1S64 Intersection1S64(Rng1S64 a, Rng1S64 b) { return R1S64(Max(a.min, b.min), Min(a.max, b.max)); }

engine_function Rng1U64 R1U64(U64 min, U64 max)
{
    Rng1U64 result = { min, max };
    if(result.max < result.min)
    {
        Swap(U64, result.min, result.max);
    }
    return result;
}
engine_function Rng1U64 Pad1U64(Rng1U64 r, U64 x) { return R1U64(r.min-x, r.max+x); }
engine_function U64 Center1U64(Rng1U64 r) { return (r.min + r.max)/2; }
engine_function B32 Contains1U64(Rng1U64 r, U64 v) { return r.min <= v && v < r.max; }
engine_function U64 Dim1U64(Rng1U64 r) { return AbsoluteValueU64(r.max - r.min); }
engine_function Rng1U64 Union1U64(Rng1U64 a, Rng1U64 b) { return R1U64(Min(a.min, b.min), Max(a.max, b.max)); }
engine_function Rng1U64 Intersection1U64(Rng1U64 a, Rng1U64 b) { return R1U64(Max(a.min, b.min), Min(a.max, b.max)); }

engine_function Rng2F32
R2F32(Vec2F32 min, Vec2F32 max)
{
    Rng2F32 result = { min, max };
    return result;
}
engine_function Rng2F32 Shift2F32(Rng2F32 r, Vec2F32 v) { r.x0 += v.x; r.y0 += v.y; r.x1 += v.x; r.y1 += v.y; return r; }
engine_function Rng2F32 Pad2F32(Rng2F32 r, F32 x) { return R2F32(Sub2F32(r.min, V2F32(x, x)), Add2F32(r.max, V2F32(x, x))); }
engine_function Vec2F32 Center2F32(Rng2F32 r) { return V2F32((r.min.x + r.max.x)/2, (r.min.y + r.max.y)/2); }
engine_function B32 Contains2F32(Rng2F32 r, Vec2F32 v) { return (r.min.x <= v.x && v.x <= r.max.x) && (r.min.y <= v.y && v.y <= r.max.y); }
engine_function Vec2F32 Dim2F32(Rng2F32 r) { return V2F32(AbsoluteValueF32(r.max.x - r.min.x), AbsoluteValueF32(r.max.y - r.min.y)); }
engine_function Rng2F32 Union2F32(Rng2F32 a, Rng2F32 b)
{
    return R2F32(V2F32(Min(a.min.x, b.min.x), Min(a.min.y, b.min.y)),
                 V2F32(Max(a.max.x, b.max.x), Max(a.max.y, b.max.y)));
}
engine_function Rng2F32 Intersection2F32(Rng2F32 a, Rng2F32 b)
{
    return R2F32(V2F32(Max(a.min.x, b.min.x), Max(a.min.y, b.min.y)),
                 V2F32(Min(a.max.x, b.max.x), Min(a.max.y, b.max.y)));
}

engine_function Rng2S64
R2S64(Vec2S64 min, Vec2S64 max)
{
    Rng2S64 result = { min, max };
    return result;
}
engine_function Rng2S64 Pad2S64(Rng2S64 r, S64 x) { return R2S64(Sub2S64(r.min, V2S64(x, x)), Add2S64(r.max, V2S64(x, x))); }
engine_function Vec2S64 Center2S64(Rng2S64 r) { return V2S64((r.min.x + r.max.x)/2, (r.min.y + r.max.y)/2); }
engine_function B32 Contains2S64(Rng2S64 r, Vec2S64 v) { return (r.min.x <= v.x && v.x < r.max.x) && (r.min.y <= v.y && v.y < r.max.y); }
engine_function Vec2S64 Dim2S64(Rng2S64 r) { return V2S64(AbsoluteValueS64(r.max.x - r.min.x), AbsoluteValueS64(r.max.y - r.min.y)); }
engine_function Rng2S64 Union2S64(Rng2S64 a, Rng2S64 b)
{
    return R2S64(V2S64(Min(a.min.x, b.min.x), Min(a.min.y, b.min.y)),
                 V2S64(Max(a.max.x, b.max.x), Max(a.max.y, b.max.y)));
}
engine_function Rng2S64 Intersection2S64(Rng2S64 a, Rng2S64 b)
{
    return R2S64(V2S64(Max(a.min.x, b.min.x), Max(a.min.y, b.min.y)),
                 V2S64(Min(a.max.x, b.max.x), Min(a.max.y, b.max.y)));
}

////////////////////////////////
//~ rjf: Matrix Constructors

engine_function Matrix3x3F32
MakeMatrix3x3F32(F32 d)
{
    Matrix3x3F32 result =
    {
        {
            {d, 0, 0},
            {0, d, 0},
            {0, 0, d},
        },
    };
    return result;
}

engine_function Matrix3x3F32
MakeTranslate3x3F32(Vec2F32 translation)
{
    Matrix3x3F32 result = MakeMatrix3x3F32(1.f);
    result.elements[2][0] = translation.x;
    result.elements[2][1] = translation.y;
    return result;
}

engine_function Matrix3x3F32
MakeScale3x3F32(Vec2F32 scale)
{
    Matrix3x3F32 result = MakeMatrix3x3F32(1.f);
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    return result;
}

engine_function Matrix3x3F32
MakeRotate3x3(F32 angle)
{
    Matrix3x3F32 result = MakeMatrix3x3F32(1.f);
    result.elements[0][0] = +Cos(angle);
    result.elements[1][0] = -Sin(angle);
    result.elements[0][1] = +Sin(angle);
    result.elements[1][1] = +Cos(angle);
    return result;
}

engine_function Matrix4x4F32
MakeMatrix4x4F32(F32 d)
{
    Matrix4x4F32 result =
    {
        {
            {d, 0, 0, 0},
            {0, d, 0, 0},
            {0, 0, d, 0},
            {0, 0, 0, d},
        }
    };
    return result;
}

engine_function Matrix4x4F32
MakeTranslate4x4F32(Vec3F32 translation)
{
    Matrix4x4F32 result = MakeMatrix4x4F32(1.f);
    result.elements[3][0] = translation.x;
    result.elements[3][1] = translation.y;
    result.elements[3][2] = translation.z;
    return result;
}

engine_function Matrix4x4F32
MakeScale4x4F32(Vec3F32 scale)
{
    Matrix4x4F32 result = MakeMatrix4x4F32(1.f);
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;
    return result;
}

engine_function Matrix4x4F32
MakePerspective4x4F32(F32 fov, F32 aspect_ratio, F32 near_z, F32 far_z)
{
    Matrix4x4F32 result = MakeMatrix4x4F32(1.f);
    F32 tan_theta_over_2 = Tan(fov / 2);
    result.elements[0][0] = 1.f / tan_theta_over_2;
    result.elements[1][1] = aspect_ratio / tan_theta_over_2;
    result.elements[2][3] = -1.f;
    result.elements[2][2] = (near_z + far_z) / (near_z - far_z);
    result.elements[3][2] = (2.f * near_z * far_z) / (near_z - far_z);
    result.elements[3][3] = 0.f;
    return result;
}

engine_function Matrix4x4F32
MakeLookAt4x4F32(Vec3F32 eye, Vec3F32 center, Vec3F32 up)
{
    Matrix4x4F32 result;
    Vec3F32 f = Normalize3F32(Sub3F32(center, eye));
    Vec3F32 s = Normalize3F32(Cross3F32(f, up));
    Vec3F32 u = Cross3F32(s, f);
    result.elements[0][0] = s.x;
    result.elements[0][1] = u.x;
    result.elements[0][2] = -f.x;
    result.elements[0][3] = 0.0f;
    result.elements[1][0] = s.y;
    result.elements[1][1] = u.y;
    result.elements[1][2] = -f.y;
    result.elements[1][3] = 0.0f;
    result.elements[2][0] = s.z;
    result.elements[2][1] = u.z;
    result.elements[2][2] = -f.z;
    result.elements[2][3] = 0.0f;
    result.elements[3][0] = -Dot3F32(s, eye);
    result.elements[3][1] = -Dot3F32(u, eye);
    result.elements[3][2] = Dot3F32(f, eye);
    result.elements[3][3] = 1.0f;
    return result;
}

////////////////////////////////
//~ rjf: Matrix Ops

engine_function Matrix3x3F32
Mul3x3F32(Matrix3x3F32 a, Matrix3x3F32 b)
{
    Matrix3x3F32 c = {0};
    for(int j = 0; j < 3; j += 1)
    {
        for(int i = 0; i < 3; i += 1)
        {
            c.elements[i][j] = (a.elements[0][j]*b.elements[i][0] +
                                a.elements[1][j]*b.elements[i][1] +
                                a.elements[2][j]*b.elements[i][2]);
        }
    }
    return c;
}

engine_function Matrix3x3F32
Scale3x3F32(Matrix3x3F32 m, F32 scale)
{
    for(int j = 0; j < 3; j += 1)
    {
        for(int i = 0; i < 3; i += 1)
        {
            m.elements[i][j] *= scale;
        }
    }
    return m;
}

engine_function Matrix4x4F32
Scale4x4F32(Matrix4x4F32 m, F32 scale)
{
    for(int j = 0; j < 4; j += 1)
    {
        for(int i = 0; i < 4; i += 1)
        {
            m.elements[i][j] *= scale;
        }
    }
    return m;
}

engine_function Matrix4x4F32
Mul4x4F32(Matrix4x4F32 a, Matrix4x4F32 b)
{
    Matrix4x4F32 c = {0};
    for(int j = 0; j < 4; j += 1)
    {
        for(int i = 0; i < 4; i += 1)
        {
            c.elements[i][j] = (a.elements[0][j]*b.elements[i][0] +
                                a.elements[1][j]*b.elements[i][1] +
                                a.elements[2][j]*b.elements[i][2] +
                                a.elements[3][j]*b.elements[i][3]);
        }
    }
    return c;
}

engine_function Matrix4x4F32
Inverse4x4F32(Matrix4x4F32 m)
{
    F32 coef00 = m.elements[2][2] * m.elements[3][3] - m.elements[3][2] * m.elements[2][3];
    F32 coef02 = m.elements[1][2] * m.elements[3][3] - m.elements[3][2] * m.elements[1][3];
    F32 coef03 = m.elements[1][2] * m.elements[2][3] - m.elements[2][2] * m.elements[1][3];
    F32 coef04 = m.elements[2][1] * m.elements[3][3] - m.elements[3][1] * m.elements[2][3];
    F32 coef06 = m.elements[1][1] * m.elements[3][3] - m.elements[3][1] * m.elements[1][3];
    F32 coef07 = m.elements[1][1] * m.elements[2][3] - m.elements[2][1] * m.elements[1][3];
    F32 coef08 = m.elements[2][1] * m.elements[3][2] - m.elements[3][1] * m.elements[2][2];
    F32 coef10 = m.elements[1][1] * m.elements[3][2] - m.elements[3][1] * m.elements[1][2];
    F32 coef11 = m.elements[1][1] * m.elements[2][2] - m.elements[2][1] * m.elements[1][2];
    F32 coef12 = m.elements[2][0] * m.elements[3][3] - m.elements[3][0] * m.elements[2][3];
    F32 coef14 = m.elements[1][0] * m.elements[3][3] - m.elements[3][0] * m.elements[1][3];
    F32 coef15 = m.elements[1][0] * m.elements[2][3] - m.elements[2][0] * m.elements[1][3];
    F32 coef16 = m.elements[2][0] * m.elements[3][2] - m.elements[3][0] * m.elements[2][2];
    F32 coef18 = m.elements[1][0] * m.elements[3][2] - m.elements[3][0] * m.elements[1][2];
    F32 coef19 = m.elements[1][0] * m.elements[2][2] - m.elements[2][0] * m.elements[1][2];
    F32 coef20 = m.elements[2][0] * m.elements[3][1] - m.elements[3][0] * m.elements[2][1];
    F32 coef22 = m.elements[1][0] * m.elements[3][1] - m.elements[3][0] * m.elements[1][1];
    F32 coef23 = m.elements[1][0] * m.elements[2][1] - m.elements[2][0] * m.elements[1][1];
    
    Vec4F32 fac0 = { coef00, coef00, coef02, coef03 };
    Vec4F32 fac1 = { coef04, coef04, coef06, coef07 };
    Vec4F32 fac2 = { coef08, coef08, coef10, coef11 };
    Vec4F32 fac3 = { coef12, coef12, coef14, coef15 };
    Vec4F32 fac4 = { coef16, coef16, coef18, coef19 };
    Vec4F32 fac5 = { coef20, coef20, coef22, coef23 };
    
    Vec4F32 vec0 = { m.elements[1][0], m.elements[0][0], m.elements[0][0], m.elements[0][0] };
    Vec4F32 vec1 = { m.elements[1][1], m.elements[0][1], m.elements[0][1], m.elements[0][1] };
    Vec4F32 vec2 = { m.elements[1][2], m.elements[0][2], m.elements[0][2], m.elements[0][2] };
    Vec4F32 vec3 = { m.elements[1][3], m.elements[0][3], m.elements[0][3], m.elements[0][3] };
    
    Vec4F32 inv0 = Add4F32(Sub4F32(Mul4F32(vec1, fac0), Mul4F32(vec2, fac1)), Mul4F32(vec3, fac2));
    Vec4F32 inv1 = Add4F32(Sub4F32(Mul4F32(vec0, fac0), Mul4F32(vec2, fac3)), Mul4F32(vec3, fac4));
    Vec4F32 inv2 = Add4F32(Sub4F32(Mul4F32(vec0, fac1), Mul4F32(vec1, fac3)), Mul4F32(vec3, fac5));
    Vec4F32 inv3 = Add4F32(Sub4F32(Mul4F32(vec0, fac2), Mul4F32(vec1, fac4)), Mul4F32(vec2, fac5));
    
    Vec4F32 sign_a = { +1, -1, +1, -1 };
    Vec4F32 sign_b = { -1, +1, -1, +1 };
    
    Matrix4x4F32 inverse;
    for(U32 i = 0; i < 4; i += 1)
    {
        inverse.elements[0][i] = inv0.elements[i] * sign_a.elements[i];
        inverse.elements[1][i] = inv1.elements[i] * sign_b.elements[i];
        inverse.elements[2][i] = inv2.elements[i] * sign_a.elements[i];
        inverse.elements[3][i] = inv3.elements[i] * sign_b.elements[i];
    }
    
    Vec4F32 row0 = { inverse.elements[0][0], inverse.elements[1][0], inverse.elements[2][0], inverse.elements[3][0] };
    Vec4F32 m0 = { m.elements[0][0], m.elements[0][1], m.elements[0][2], m.elements[0][3] };
    Vec4F32 dot0 = Mul4F32(m0, row0);
    F32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    F32 one_over_det = 1 / dot1;
    
    return Scale4x4F32(inverse, one_over_det);
}

engine_function Matrix4x4F32
RemoveRotation4x4F32(Matrix4x4F32 mat)
{
    Vec3F32 scale =
    {
        Length3F32(V3F32(mat.elements[0][0], mat.elements[0][1], mat.elements[0][2])),
        Length3F32(V3F32(mat.elements[1][0], mat.elements[1][1], mat.elements[1][2])),
        Length3F32(V3F32(mat.elements[2][0], mat.elements[2][1], mat.elements[2][2])),
    };
    mat.elements[0][0] = scale.x;
    mat.elements[1][0] = 0.f;
    mat.elements[2][0] = 0.f;
    mat.elements[0][1] = 0.f;
    mat.elements[1][1] = scale.y;
    mat.elements[2][1] = 0.f;
    mat.elements[0][2] = 0.f;
    mat.elements[1][2] = 0.f;
    mat.elements[2][2] = scale.z;
    return mat;
}

////////////////////////////////
//~ rjf: Lists

engine_function void
Rng1U64ListPushNode(Rng1U64List *list, Rng1U64Node *node)
{
    QueuePush(list->first, list->last, node);
    list->count += 1;
    list->total_count += Dim1U64(node->v);
}

engine_function void
Rng1U64ListPushNodeFront(Rng1U64List *list, Rng1U64Node *node)
{
    QueuePushFront(list->first, list->last, node);
    list->count += 1;
    list->total_count += Dim1U64(node->v);
}

engine_function void
Rng1U64ListPush(M_Arena *arena, Rng1U64List *list, Rng1U64 rng)
{
    Rng1U64Node *n = PushArrayZero(arena, Rng1U64Node, 1);
    n->v = rng;
    Rng1U64ListPushNode(list, n);
}

engine_function void
Rng1U64ListPushFront(M_Arena *arena, Rng1U64List *list, Rng1U64 rng)
{
    Rng1U64Node *n = PushArrayZero(arena, Rng1U64Node, 1);
    n->v = rng;
    Rng1U64ListPushNodeFront(list, n);
}

////////////////////////////////
//~ rjf: Miscellaneous Ops

engine_function Vec3F32
HSVFromRGB(Vec3F32 rgb)
{
    F32 r = rgb.r;
    F32 g = rgb.g;
    F32 b = rgb.b;
    F32 k = 0.f;
    if(g < b)
    {
        F32 swap = b;
        b= g;
        g = swap;
        k = -1.f;
    }
    if(r < g)
    {
        F32 swap = r;
        r = g;
        g = swap;
        k = -2.f / 6.f - k;
    }
    F32 chroma = r - (g < b ? g : b);
    Vec3F32 result;
    result.x = AbsoluteValueF32(k + (g - b) / (6.f * chroma + 1e-20f));
    result.y = chroma / (r + 1e-20f);
    result.z = r;
    return result;
}

engine_function Vec3F32
RGBFromHSV(Vec3F32 hsv)
{
    Vec3F32 rgb = {0};
    
    if(hsv.y == 0.0f)
    {
        rgb.r = rgb.g = rgb.b = hsv.z;
    }
    else
    {
        F32 h = hsv.x;
        F32 s = hsv.y;
        F32 v = hsv.z;
        
        if(h >= 1.f)
        {
            h -= 10 * 1e-6f;
        }
        
        if(s >= 1.f)
        {
            s -= 10 * 1e-6f;
        }
        
        if(v >= 1.f)
        {
            v -= 10 * 1e-6f;
        }
        
        h = Mod(h, 1.f) / (60.f/360.f);
        int i = (int)h;
        F32 f = h - (F32)i;
        F32 p = v * (1.0f - s);
        F32 q = v * (1.0f - s * f);
        F32 t = v * (1.0f - s * (1.0f - f));
        
        switch (i)
        {
            case 0: { rgb.r = v; rgb.g = t; rgb.b = p; break; }
            case 1: { rgb.r = q; rgb.g = v; rgb.b = p; break; }
            case 2: { rgb.r = p; rgb.g = v; rgb.b = t; break; }
            case 3: { rgb.r = p; rgb.g = q; rgb.b = v; break; }
            case 4: { rgb.r = t; rgb.g = p; rgb.b = v; break; }
            case 5: { default: rgb.r = v; rgb.g = p; rgb.b = q; break; }
        }
    }
    
    return rgb;
}

engine_function Vec4F32
Vec4F32FromHexRGBA(U32 hex)
{
    Vec4F32 result =
    {
        (F32)((hex & 0xff000000) >> 24) / 255.f,
        (F32)((hex & 0x00ff0000) >> 16) / 255.f,
        (F32)((hex & 0x0000ff00) >>  8) / 255.f,
        (F32)((hex & 0x000000ff) >>  0) / 255.f,
    };
    return result;
}

engine_function F32
MillisecondsFromMicroseconds(U64 microseconds)
{
    F32 milliseconds = (F32)((F64)microseconds / 1000.0);
    return milliseconds;
}

engine_function U64
MicrosecondsFromMilliseconds(F32 milliseconds)
{
    U64 microseconds = (U64)((F64)milliseconds * 1000.0);
    return microseconds;
}
