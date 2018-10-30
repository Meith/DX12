cbuffer per_frame_const_buffers : register(b0)
{
        matrix mvp : packoffset(c0);
}

struct vertex_input
{
        float4 position : POSITION;
        float4 colour   : COLOR;
        float2 uv       : TEXCOORD;
};

struct vertex_output
{
        float4 colour   : COLOR;
        float2 uv       : TEXCOORD;
        float4 position : SV_POSITION;
};

vertex_output main(vertex_input vi)
{
        vertex_output vo;
        vo.colour = vi.colour;
        vo.uv = vi.uv;
        vo.position = mul(mvp, vi.position);

        return vo;
}