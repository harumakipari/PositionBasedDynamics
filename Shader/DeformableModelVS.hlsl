#include "DeformableModel.hlsli"
#include "Constants.hlsli"
DEFORMABLE_VS_OUT main(DEFORMABLE_VS_IN vin)
{
    DEFORMABLE_VS_OUT vout;

    vin.position.w = 1;
    vout.position = mul(vin.position, viewProjection);
    vout.wPosition = vin.position;

    vin.normal.w = 0;
    vout.wNormal = normalize(mul(vin.normal, deformationRotation));

    float sigma = vin.tangent.w;
    vin.tangent.w = 0;
    vout.wTangent = normalize(mul(vin.tangent, deformationRotation));
    vout.wTangent.w = sigma;

    vout.texcoord = vin.texcoord;

    return vout;
}
