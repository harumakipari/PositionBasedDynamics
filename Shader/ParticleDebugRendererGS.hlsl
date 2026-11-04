#include "Constants.hlsli"
#include "ParticleDebug.hlsli"

[maxvertexcount(4)]
void main(point VS_OUT input[1], inout TriangleStream<VS_OUT> output)
{
    VS_OUT vertex = input[0];

    // Convert the requested pixel size into clip-space offsets. Multiplying
    // by w preserves a constant on-screen size at different depths.
    float2 half_size_ndc = (input[0].size * 0.5f) * 2.0f / iResolution;
    float2 half_size_clip = half_size_ndc * input[0].position.w;

    vertex.position = input[0].position + float4(-half_size_clip.x, -half_size_clip.y, 0.0f, 0.0f);
    output.Append(vertex);
    vertex.position = input[0].position + float4(-half_size_clip.x,  half_size_clip.y, 0.0f, 0.0f);
    output.Append(vertex);
    vertex.position = input[0].position + float4( half_size_clip.x, -half_size_clip.y, 0.0f, 0.0f);
    output.Append(vertex);
    vertex.position = input[0].position + float4( half_size_clip.x,  half_size_clip.y, 0.0f, 0.0f);
    output.Append(vertex);
}
