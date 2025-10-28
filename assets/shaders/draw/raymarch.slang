
[[vk::binding(0)]]
Sampler3D sdf_sampler;

struct PushConstants {
    float4x4 transform;
    float4x4 view_proj;
    float3 camera_pos;
    float3 field_size;
}

struct Vertex {
    float3 position : POSITION;
    float2 uv : UV;
    float3 normal;
}

struct VertexOutput {
    float4 position : SV_Position;
    float3 obj_pos;
    float4 world_pos;
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
    out.obj_pos = vertex.position;
    out.world_pos = mul(pc.transform, float4(vertex.position, 1.0f));
    out.uv = vertex.uv;
    out.normal = vertex.normal;

    return out;
}

bool IsInsideAABB(float3 pos) {
    return pos.x >= -1 && pos.x <= 1 && pos.y >= -1 && pos.y <= 1 && pos.z >= -1 && pos.z <= 1;
}

float4 DensityAtPosition(float3 pos) {
    if (pos.x >= -1 && pos.x <= 1 && pos.y >= -1 && pos.y <= 1 && pos.z >= -1 && pos.z <= 1) {
        let val = sdf_sampler.Sample(0.5f * (pos + 1));

        float d = val.x;

        let cintensity = 0.4;
        let half_interval = 5;
        if (d < 0 && d > -half_interval) {
            return float4(0.0, 0.0, cintensity, 1.0);
        } else if (d >= 0 && d < half_interval) {
            return float4(0.0, cintensity, 0.0, 1.0);
        }
    }

    return float4(0.0f);
}

float4 ComputeForwardLightRaymarch(float3 origin,
                                   float3 direction,
                                   PushConstants pc,
                                   VertexOutput in) {
    let n_steps = 10000;
    let step = 0.001;
    float4 light = 0.0f;
    float sign = 1.0;  // front ? 1 : -1;

    let initial_pos = origin;

    for (int i = 0; i < n_steps; i++) {
        if (!IsInsideAABB(origin)) {
            break;
        }
        light += step * DensityAtPosition(origin);
        origin += sign * direction * step;
    }

    return light;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input, uniform PushConstants pc) {
    FragmentOutput out;

    out.color = ComputeForwardLightRaymarch(
        input.world_pos.xyz, normalize(input.world_pos.xyz - pc.camera_pos.xyz), pc, input);
    return out;
}
