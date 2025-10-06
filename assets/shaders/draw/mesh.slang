

struct PushConstants {
    float4x4 transform;
    float4x4 view_proj;
}

struct Vertex {
    float3 position : POSITION;
    float2 uv : UV;
}

struct VertexOutput {
    float4 position : SV_Position;
    float2 uv : UV;
};

struct FragmentOutput {
    float4 color : SV_Target;
}

[shader("vertex")]
VertexOutput vertex_main(Vertex vertex, uniform PushConstants pc) {
    VertexOutput out;

    out.position = mul(mul(pc.transform, pc.view_proj), float4(vertex.position, 1.0f));
    out.uv = vertex.uv;

    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input, uniform PushConstants pc) {
    FragmentOutput out;
    out.color = float4(1.0f);
    return out;
}
