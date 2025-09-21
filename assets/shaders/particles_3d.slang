import colormap;

static const float v_max = 6.5f;
static const float radius = 0.5;
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

    float3 n;
    n.xy = input.uv * 2.0f - 1.0f;
    let r2 = dot(n.xy, n.xy);

    if (r2 > 1.0f) {
        discard;
    }

    n.z = -sqrt(1.0 - r2);

    var light_dir = float3(0.0, 1.0, -1.0);
    light_dir = normalize(light_dir);
    float diffuse = max(0.0, dot(n, light_dir));

    out.color = float4(input.color.r, input.color.g, input.color.b, 1.0f);
    out.color.xyz *= diffuse;
    return out;
}
