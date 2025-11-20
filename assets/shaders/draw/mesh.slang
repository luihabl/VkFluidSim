

struct PushConstants {
    float4x4 transform;
    float4x4 view_proj;
}

struct Vertex {
    float3 position : POSITION;
    float2 uv : UV;
    float3 normal;
}

struct VertexOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    float3 normal;
};

struct FragmentOutput {
    float4 color : SV_Target;
}

[shader("vertex")]
VertexOutput vertex_main(Vertex vertex, uniform PushConstants pc) {
    VertexOutput out;

    out.position = mul(mul(pc.view_proj, pc.transform), float4(vertex.position, 1.0f));
    out.uv = vertex.uv;
    out.normal = vertex.normal;

    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input, uniform PushConstants pc) {
    FragmentOutput out;

    let base_color = float3(1.0f);
    var light = dot(normalize(input.normal), normalize(float3(0.0f, 1.0f, 0.0f)));
    out.color = float4(0.5 * base_color + 0.8 * light, 1.0f);
    return out;
}
