import colormap;

static const float v_max = 6.5f;
static const float radius = 0.2;

struct Vertex {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
}

struct VertexOutput {
    float4 position : SV_Position;
    float4 color;
    float2 uv : UV;
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
    out.uv = vertex.uv;

    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input) {
    FragmentOutput out;

    let uv_centered = input.uv - float2(0.5, 0.5);
    float sqr_dist = dot(uv_centered, uv_centered);

    if (sqr_dist <= radius * radius) {
        out.color = input.color;
        return out;
    } else {
        out.color = float4(0, 0, 0, 0);
        return out;
    }
}
