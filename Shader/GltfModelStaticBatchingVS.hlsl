#include "GltfModel.hlsli"

cbuffer DamageConstants : register(b12)
{
    float3 HitPosition;
    float Radius;

    float3 HitNormal;
    float Strength;
};

VS_OUT main(float4 position : POSITION, float4 normal : NORMAL, float4 tangent : TANGENT, float2 texcoord[2] : TEXCOORD)
{
    VS_OUT vout;
    position.w = 1;
    vout.position = mul(position, mul(world, viewProjection));
    vout.wPosition = mul(position, world);
#if 0
    vout.wPosition = mul(position, world);
#else
    float4 worldPos = mul(position, world);
    float d = distance(worldPos.xyz, HitPosition);
    float w = saturate(1.0 - d / Radius);
    //w *= w;
    w = smoothstep(1.0f, 0.0f, d / Radius);
    worldPos.xyz += HitNormal * Strength * w;
    vout.wPosition = worldPos;
    vout.position = mul(worldPos, viewProjection);
#endif


    normal.w = 0;
    vout.wNormal = normalize(mul(normal, world));
    //vout.wNormal.xyz = normalize(mul(gbuffer1Normal, inverseTransposeWorld).xyz);
    vout.wNormal.w = 0;

    float sigma = tangent.w;
    tangent.w = 0;
    vout.wTangent = normalize(mul(tangent, world));
    //vout.wTangent.xyz = normalize(mul(tangent, inverseTransposeWorld).xyz);
    vout.wTangent.w = sigma;

    vout.texcoord = texcoord[0];

    return vout;
}
