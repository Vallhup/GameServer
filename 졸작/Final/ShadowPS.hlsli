struct PS_IN
{
    float4 pos : SV_POSITION;
};

void PSMain(PS_IN input)
{
    // GPU가 자동으로 depth 값을 shadow map에 기록함
}