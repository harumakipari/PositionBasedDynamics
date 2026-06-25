struct VS_IN
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};