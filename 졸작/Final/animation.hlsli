
#include "math.hlsli"

cbuffer AnimationParams : register(b2)
{
    int aBoneCount; 
    int aCurrentFrame;  
    int aNextFrame; 
    float aRatio; 
    int aAnimationOffset;
    
    int isBlending;
    int aPrevCurrentFrame;
    int aPrevNextFrame;
    float aPrevRatio;
    int aPrevAnimationOffset;
    float aBlendRatio;
    
    float padding;
}

struct AnimFrameParams
{
    float4 scale;
    float4 rotation;
    float4 translation;
};

StructuredBuffer<AnimFrameParams> aBoneFrame : register(t1);
StructuredBuffer<matrix> aOffset : register(t2);
RWStructuredBuffer<matrix> aFinal : register(u0);

matrix CalculateBoneMatrix(int boneIndex, int currentFrame, int nextFrame, float ratio, int animOffset)
{
    int currentFrameIndex = currentFrame - 1;
    int nextFrameIndex = nextFrame - 1;
    
    uint idx = animOffset + (aBoneCount * currentFrameIndex) + boneIndex;
    uint nextIdx = animOffset + (aBoneCount * nextFrameIndex) + boneIndex;

    float4 scale = lerp(aBoneFrame[idx].scale, aBoneFrame[nextIdx].scale, ratio);
    float4 rotation = QuaternionSlerp(aBoneFrame[idx].rotation, aBoneFrame[nextIdx].rotation, ratio);
    float4 translation = lerp(aBoneFrame[idx].translation, aBoneFrame[nextIdx].translation, ratio);

    matrix matBone = MatrixAffineTransformation(scale, rotation, translation);

    return mul(aOffset[boneIndex], matBone);
}

[numthreads(256, 1, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    if (aBoneCount <= threadIdx.x)
        return;
    
    if (isBlending)
    {
        matrix currentmatrix = CalculateBoneMatrix(threadIdx.x, aCurrentFrame, aNextFrame, aRatio, aAnimationOffset);
        matrix prevmatrix = CalculateBoneMatrix(threadIdx.x, aPrevCurrentFrame, aPrevNextFrame, aPrevRatio, aPrevAnimationOffset);
        
        aFinal[threadIdx.x] = lerp(prevmatrix, currentmatrix, aBlendRatio);

    }
    else
        aFinal[threadIdx.x] = CalculateBoneMatrix(threadIdx.x, aCurrentFrame, aNextFrame, aRatio, aAnimationOffset);
}
