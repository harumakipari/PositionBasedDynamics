struct DEFORMABLE_VS_IN
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};
struct DEFORMABLE_VS_OUT
{
    float4 position : SV_POSITION;
    float4 wPosition : POSITION;
    float4 wNormal : NORMAL;
    float4 wTangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

cbuffer PRIMITIVE_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 deformationRotation;
    int material;
    bool hasTangent;
    int skin;
    int pad;
};


struct TextureInfo
{
    int index;
    int texcoord;
};

struct NormalTextureInfo
{
    int index;
    int texcoord;
    float scale;
};

struct OcclusionTextureInfo
{
    int index;
    int texcoord;
    float strength;
};

struct PbrMetallicRoughness
{
    float4 baseColorFactor;
    TextureInfo basecolorTexture;
    float metallicFactor;
    float roughnessFactor;
    TextureInfo metallicRoughnessTexture;
};

struct MaterialConstants
{
    float3 emissiveFactor; // length 3. default [0, 0, 0]
    int alphaMode; // "OPAQUE" : 0, "MASK" : 1, "BLEND" : 2 
    float alphaCutoff; // default 0.5
    bool doubleSided; // default false;
    
    PbrMetallicRoughness pbrMetallicRoughness;
    
    NormalTextureInfo normalTexture;
    OcclusionTextureInfo occlusionTexture;
    TextureInfo emissiveTexture;
};

StructuredBuffer<MaterialConstants> materials : register(t0);