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
    float4 particle_pos;
};

struct FragmentOutput {
    float4 color : SV_Target;
    float depth : SV_Depth;
}

struct InputConstants {
    float4x4 model_view;
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
    out.position = mul(mul(pc.proj, pc.model_view), float4(offset, 1.0f)) +
                   mul(pc.proj, float4(vertex.position, 1.0f));
    out.particle_pos = mul(pc.model_view, float4(offset, 1.0f));
    out.color = IDL::Mac_Style::colormap(grad);
    out.uv = vertex.uv;

    return out;
}

static const float3 light_dir = normalize(float3(0.0, 1.0, 1.0));

[shader("fragment")]
FragmentOutput frag_main(uint id: SV_VulkanInstanceID,
                         VertexOutput input,
                         uniform InputConstants pc) {
    FragmentOutput out;

    float3 n;
    n.xy = input.uv * 2.0f - 1.0f;
    let r2 = dot(n.xy, n.xy);

    if (r2 > 1.0f) {
        discard;
    }

    n.z = sqrt(1.0 - r2);

    float diffuse = max(0.0, dot(n, light_dir));
    out.color = float4(input.color.r, input.color.g, input.color.b, 1.0f);
    out.color.xyz *= diffuse;

    let pixel_pos = float4(input.particle_pos.xyz + radius * n, 1.0f);
    let clip_space_pos = mul(pc.proj, pixel_pos);

    out.depth = clip_space_pos.z / clip_space_pos.w;

    return out;
}
