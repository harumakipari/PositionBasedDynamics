#include "Constants.hlsli"
#include "ParticleDebug.hlsli"

VS_OUT main(VS_IN input)
{
    VS_OUT output;

    output.position =
        mul(float4(input.position, 1.0f),
            viewProjection);

    output.color = input.color;

    return output;
}