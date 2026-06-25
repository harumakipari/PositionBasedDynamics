#include "ParticleDebug.hlsli"

float4 main(VS_OUT pout) : SV_TARGET
{
    return float4(pout.color, 1.0f);
}