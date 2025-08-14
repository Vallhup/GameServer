
#include "math.hlsli"

cbuffer AnimationParams : register(b2)
{
    int g_boneCount; 
    int g_currentFrame;  
    int g_nextFrame; 
    float g_ratio; 
    int animationOffset;
}

struct AnimFrameParams
{
    float4 scale;
    float4 rotation;
    float4 translation;
};

StructuredBuffer<AnimFrameParams> g_bone_frame : register(t10);
StructuredBuffer<matrix> g_offset : register(t11);
RWStructuredBuffer<matrix> g_final : register(u0);

// ComputeAnimation
// g_int_0 : BoneCount
// g_int_1 : CurrentFrame
// g_int_2 : NextFrame
// g_float_0 : Ratio
[numthreads(256, 1, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    if (g_boneCount <= threadIdx.x)
        return;

    int boneCount = g_boneCount;
    int currentFrame = g_currentFrame;
    int nextFrame = g_nextFrame;
    float ratio = g_ratio;

    uint idx = animationOffset + (boneCount * currentFrame) + threadIdx.x;
    uint nextIdx = animationOffset + (boneCount * nextFrame) + threadIdx.x;

    float4 quaternionZero = float4(0.f, 0.f, 0.f, 1.f);

    float4 scale = lerp(g_bone_frame[idx].scale, g_bone_frame[nextIdx].scale, ratio);
    float4 rotation = QuaternionSlerp(g_bone_frame[idx].rotation, g_bone_frame[nextIdx].rotation, ratio);
    float4 translation = lerp(g_bone_frame[idx].translation, g_bone_frame[nextIdx].translation, ratio);

    matrix matBone = MatrixAffineTransformation(scale, quaternionZero, rotation, translation);

    g_final[threadIdx.x] = mul(g_offset[threadIdx.x], matBone);
}
