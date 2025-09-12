cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
};

cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    int useInstancing;
    uint materialIndex;
    int objPadding;
};

StructuredBuffer<matrix> instanceTransforms : register(t0, space2);
StructuredBuffer<matrix> finalBoneTransforms : register(t3);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
};

void Skinning(inout float3 pos, inout float3 normal, inout float3 tangent, inout float4 weight, inout float4 indices)
{
    float3 skinnedPos = float3(0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float3 skinnedTangent = float3(0, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
        if (weight[i] == 0.f)
            continue;

        int boneIdx = (int) indices[i];
        matrix matBone = finalBoneTransforms[boneIdx];

        skinnedPos += (mul(float4(pos, 1.f), matBone) * weight[i]).xyz;
        skinnedNormal += (mul(float4(normal, 0.f), matBone) * weight[i]).xyz;
        skinnedTangent += (mul(float4(tangent, 0.f), matBone) * weight[i]).xyz;
    }

    pos = skinnedPos;
    normal = normalize(skinnedNormal);
    tangent = normalize(skinnedTangent);
}

VS_OUT VSMain(VS_IN input, uint instanceID : SV_InstanceID)
{
    VS_OUT output;
    
    float3 modifiedPos = input.pos;
    float3 modifiedNormal = input.normal;
    float3 modifiedTangent = input.tangent;
    
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    if (useInstancing)
        hasAnimation = false;
    
    if (hasAnimation)
    {
        Skinning(modifiedPos, modifiedNormal, modifiedTangent, input.weights, input.indices);
    }
    
    matrix worldMatrix;
    
    if (useInstancing)
        worldMatrix = instanceTransforms[instanceID];
    else
        worldMatrix = world;
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    
    output.color = input.color;
    output.uv = input.uv;
    output.normal = normalize(mul(float4(modifiedNormal, 0.0f), worldMatrix).xyz);
    output.tangent = normalize(mul(float4(modifiedTangent, 0.0f), worldMatrix).xyz);
    output.weights = input.weights;
    output.indices = input.indices;
    output.materialIndex = materialIndex;
    
    return output;
}