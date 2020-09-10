cbuffer per_frame_const_buffers : register(b0)
{
        matrix mvp : packoffset(c0);
}

struct vertex_input
{
        float3 position : POSITION;
        float2 uv       : TEXCOORD;
};

struct vertex_output
{
        float2 uv       : TEXCOORD;
        float4 position : SV_POSITION;
};

vertex_output main(vertex_input vi)
{
        vertex_output vo;
        vo.uv = vi.uv;
        vo.position = mul(mvp, float4(vi.position, 1.0f));

        return vo;
}