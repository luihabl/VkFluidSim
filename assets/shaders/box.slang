
struct VertexOutput {
    float4 position : SV_Position;
    float4 color;
};

struct PushConstants {
    float4x4 matrix;
    float4 color;
}

struct FragmentOutput {
    float4 color : SV_Target;
}

static const float2 coords[] = {
    float2(0, 0), float2(1, 0), float2(1, 0), float2(1, 1),
    float2(1, 1), float2(0, 1), float2(0, 1), float2(0, 0),
};

[shader("vertex")]
VertexOutput vertex_main(uint vertex_id: SV_VertexID, uniform PushConstants pc) {
    VertexOutput out;

    out.position = mul(pc.matrix, float4(coords[vertex_id], 0.0f, 1.0f));
    out.color = pc.color;
    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input) {
    FragmentOutput out;
    out.color = input.color;
    return out;
}
