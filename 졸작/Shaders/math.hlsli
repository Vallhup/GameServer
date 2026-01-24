#ifndef MATH_HLSLI
#define MATH_HLSLI

float4 VectorPermute(uint PermuteX, uint PermuteY, uint PermuteZ, uint PermuteW, in float4 V1, in float4 V2)
{
    float4 Ptr[2] = { V1, V2 };

    float4 Result = (float4) 0.f;

    const uint i0 = PermuteX & 3;
    const uint vi0 = PermuteX >> 2;
    Result[0] = Ptr[vi0][i0];

    const uint i1 = PermuteY & 3;
    const uint vi1 = PermuteY >> 2;
    Result[1] = Ptr[vi1][i1];

    const uint i2 = PermuteZ & 3;
    const uint vi2 = PermuteZ >> 2;
    Result[2] = Ptr[vi2][i2];

    const uint i3 = PermuteW & 3;
    const uint vi3 = PermuteW >> 2;
    Result[3] = Ptr[vi3][i3];

    return Result;
}

matrix MatrixRotationQuaternion(in float4 Quaternion)
{
    float4 Constant1110 = float4(1.f, 1.f, 1.f, 0.f);

    float4 Q0 = Quaternion + Quaternion;
    float4 Q1 = Quaternion * Q0;

    float4 V0 = VectorPermute(1, 0, 0, 7, Q1, Constant1110);
    float4 V1 = VectorPermute(2, 2, 1, 7, Q1, Constant1110);
    float4 R0 = Constant1110 - V0;
    R0 = R0 - V1;

    V0 = float4(Quaternion[0], Quaternion[0], Quaternion[1], Quaternion[3]);
    V1 = float4(Q0[2], Q0[1], Q0[2], Q0[3]);
    V0 = V0 * V1;

    V1 = float4(Quaternion.w, Quaternion.w, Quaternion.w, Quaternion.w);
    float4 V2 = float4(Q0[1], Q0[2], Q0[0], Q0[3]);
    V1 = V1 * V2;

    float4 R1 = V0 + V1;
    float4 R2 = V0 - V1;

    V0 = VectorPermute(1, 4, 5, 2, R1, R2);
    V1 = VectorPermute(0, 6, 0, 6, R1, R2);

    matrix M = (matrix) 0.f;
    M._11_12_13_14 = VectorPermute(0, 4, 5, 3, R0, V0);
    M._21_22_23_24 = VectorPermute(6, 1, 7, 3, R0, V0);
    M._31_32_33_34 = VectorPermute(4, 5, 2, 3, R0, V1);
    M._41_42_43_44 = float4(0.f, 0.f, 0.f, 1.f);
    return M;
}


matrix MatrixAffineTransformation(in float4 Scaling, in float4 RotationQuaternion, in float4 Translation)
{
    float3 RotationOrigin = (float3) 0.f;
    
    matrix MScaling = (matrix) 0.f;
    MScaling._11_22_33 = Scaling.xyz;
    float4 VRotationOrigin = float4(RotationOrigin, 0.f);
    matrix MRotation = MatrixRotationQuaternion(RotationQuaternion);
    float4 VTranslation = float4(Translation.xyz, 0.f);

    matrix M = MScaling;
    M._41_42_43_44 = M._41_42_43_44 - VRotationOrigin;
    M = mul(M, MRotation);
    M._41_42_43_44 = M._41_42_43_44 + VRotationOrigin;
    M._41_42_43_44 = M._41_42_43_44 + VTranslation;
    return M;
}

float4 QuaternionNlerp(float4 Q1, float4 Q2, float t)
{
    float dotResult = dot(Q1, Q2);
    if (dotResult < 0.0f)
        Q2 = -Q2;
    
    float4 result = lerp(Q1, Q2, t);
    
    return normalize(result);

}

#endif