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
    int hasAlpha;
    int padding;
};

StructuredBuffer<matrix> instanceTransforms : register(t0, space1);
StructuredBuffer<matrix> finalBoneTransforms : register(t10);

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
};

// 루키스 방식의 스키닝 함수
void Skinning(inout float3 pos, inout float3 normal, inout float3 tangent,
    inout float4 weight, inout float4 indices)
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
    
    // 애니메이션 적용 체크
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    // 인스턴싱 모드에서는 애니메이션 비활성화
    if (useInstancing)
        hasAnimation = false;
    
    if (hasAnimation)
    {
        Skinning(modifiedPos, modifiedNormal, modifiedTangent, input.weights, input.indices);
    }
    
    // 월드 변환
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
    output.normal = modifiedNormal;
    output.tangent = modifiedTangent;
    output.weights = input.weights;
    output.indices = input.indices;
    
    return output;
}