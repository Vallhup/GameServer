#include "ShaderResources.hlsli"
#include "math.hlsli"

void GetInterpolatedSRT(int boneIndex, int currentFrame, int nextFrame, float ratio, int animOffset,
                        out float4 outS, out float4 outR, out float4 outT)
{
    int currentFrameIndex = currentFrame - 1;
    int nextFrameIndex = nextFrame - 1;
    
    uint idx = animOffset + (aBoneCount * currentFrameIndex) + boneIndex;
    uint nextIdx = animOffset + (aBoneCount * nextFrameIndex) + boneIndex;
    
    outS = lerp(aBoneFrame[idx].scale, aBoneFrame[nextIdx].scale, ratio);
    outR = QuaternionNlerp(aBoneFrame[idx].rotation, aBoneFrame[nextIdx].rotation, ratio);
    outT = lerp(aBoneFrame[idx].translation, aBoneFrame[nextIdx].translation, ratio);
}

[numthreads(256, 1, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    if (aBoneCount <= threadIdx.x)
        return;
    
    int boneIdx = threadIdx.x;
    
    float4 finalS, finalR, finalT;
    float4 s1, r1, t1;
    GetInterpolatedSRT(boneIdx, aCurrentFrame, aNextFrame, aRatio, aAnimationOffset, s1, r1, t1);
    
    if (isBlending)
    {
        float4 s2, r2, t2;
        GetInterpolatedSRT(boneIdx, aPrevCurrentFrame, aPrevNextFrame, aPrevRatio, aPrevAnimationOffset, s2, r2, t2);
   
        finalS = lerp(s2, s1, aBlendRatio);
        finalR = QuaternionNlerp(r2, r1, aBlendRatio);
        finalT = lerp(t2, t1, aBlendRatio);
    }
    else
    {
        finalS = s1;
        finalR = r1;
        finalT = t1;
    }
    
    matrix matBone = MatrixAffineTransformation(finalS, finalR, finalT);
    
    aFinal[boneIdx] = mul(aOffset[boneIdx], matBone);
}
