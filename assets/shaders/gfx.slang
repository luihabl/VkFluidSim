import colormap;

static const float v_max = 6.5f;

struct Vertex {
    float3 position : POSITION;
    float4 color : COLOR;
}

struct VertexOutput {
    float4 position : SV_Position;
    float4 color;
};

struct FragmentOutput {
    float4 color : SV_Target;
}

struct InputConstants {
    float4x4 matrix;
    float2* position;
    float2* velocity;
}

[shader("vertex")]
VertexOutput vertex_main(uint id: SV_VulkanInstanceID, Vertex vertex, uniform InputConstants pc) {
    let d = pc.position[id];
    let offset = float3(d, 0.0f);

    let v = pc.velocity[id];
    float grad = clamp(length(v) / v_max, 0.0f, 1.0f);

    VertexOutput out;
    out.position = mul(pc.matrix, float4(vertex.position + offset, 1.0f));
    out.color = IDL::Mac_Style::colormap(grad);

    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input) {
    FragmentOutput out;
    out.color = input.color;
    return out;
}
