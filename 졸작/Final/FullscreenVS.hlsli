struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    
    float2 positions[6] =
    {
        float2(-1, -1), // 왼쪽 아래  
        float2(-1, 1), // 왼쪽 위
        float2(1, -1), // 오른쪽 아래
        float2(1, -1), // 오른쪽 아래 (두 번째 삼각형)
        float2(-1, 1), // 왼쪽 위
        float2(1, 1) // 오른쪽 위
    };
    
    float2 uvs[6] =
    {
        float2(0, 1), // 왼쪽 아래
        float2(0, 0), // 왼쪽 위  
        float2(1, 1), // 오른쪽 아래
        float2(1, 1), // 오른쪽 아래
        float2(0, 0), // 왼쪽 위
        float2(1, 0) // 오른쪽 위
    };
    
    output.pos = float4(positions[vertexID], 0, 1);
    output.uv = uvs[vertexID];
    
    return output;
}