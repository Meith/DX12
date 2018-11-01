cbuffer per_frame_const_buffers : register(b0)
{
        float time;
};

RWTexture2D<float4> colour_tex : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID,
        uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
        colour_tex[DTid.xy] *= sin(50.0f);
}