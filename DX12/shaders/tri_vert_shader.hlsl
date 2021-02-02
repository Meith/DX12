cbuffer per_object_const_buffers : register(b0)
{
    matrix world_transform : packoffset(c0);
}

cbuffer per_frame_const_buffers : register(b1)
{
        matrix pv : packoffset(c0);
}

struct vertex_input
{
        float3 position : POSITION;
        float3 normal   : NORMAL;
        float2 texcoord : TEXCOORD;
        float4 tangent  : TANGENT;
};

struct vertex_output
{
        float3 normal   : NORMAL;
        float2 texcoord : TEXCOORD;
        float4 tangent  : TANGENT;
        float4 position : SV_POSITION;
};

vertex_output main(vertex_input vi)
{
        vertex_output vo;
        float4 world_pos = mul(world_transform, float4(vi.position, 1.0f));
        vo.position = mul(pv, world_pos);
        vo.normal = vi.normal;
        vo.texcoord = vi.texcoord;
        vo.tangent = vi.tangent;
        return vo;
}