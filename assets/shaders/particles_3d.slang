import colormap;

static const float v_max = 6.5f;
static const float radius = 0.15;
static const float smooth_delta = 0.001f;

struct Vertex {
    float3 position : POSITION;
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
    float4x4 proj;
    float3* position;
    float3* velocity;
}

[shader("vertex")]
VertexOutput vertex_main(uint id: SV_VulkanInstanceID, Vertex vertex, uniform InputConstants pc) {
    var offset = pc.position[id];

    let v = pc.velocity[id];
    float grad = clamp(length(v) / v_max, 0.0f, 1.0f);

    VertexOutput out;
    out.position =
        mul(pc.matrix, float4(offset, 1.0f)) + mul(pc.proj, float4(vertex.position, 1.0f));
    out.color = IDL::Mac_Style::colormap(grad);
    out.uv = vertex.uv;

    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input) {
    FragmentOutput out;

    // let uv_centered = input.uv - 0.5;
    // let squared_dist = dot(uv_centered, uv_centered);
    // let r = radius * radius;
    let intensity = 1.0f;  // 1 - smoothstep(r - smooth_delta, r + smooth_delta, squared_dist);

    out.color = float4(input.color.r, input.color.g, input.color.b, intensity);
    return out;
}
