
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

float DensityAtPosition(float3 pos) {
    if (pos.x >= -1 && pos.x <= 1 && pos.y >= -1 && pos.y <= 1 && pos.z >= -1 && pos.z <= 1) {
        let val = sdf_sampler.Sample(0.5f * (pos + 1));
        float distance = abs(val.x);

        return distance;

    } else {
        return 0.0f;
    }
}

float4 ComputeForwardLightRaymarch(float3 origin,
                                   float3 direction,
                                   PushConstants pc,
                                   VertexOutput in) {
    let step = 0.001;
    float light = 0.0f;
    float sign = 1.0;  // front ? 1 : -1;

    let initial_pos = origin;

    for (int i = 0; i < 1000; i++) {
        if (!IsInsideAABB(origin)) {
            break;
        }
        light += 20 * step * DensityAtPosition(origin);
        origin += sign * direction * step;
    }

    return light;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input, uniform PushConstants pc) {
    FragmentOutput out;

    /*
    TODO:
        - Add 3d texture
        - Convert position to uvw
        - Try to fix the sampling when points are behind the screen

    */

    // let dir = out.world_pos

    // let base_color = float3(1.0f);
    // var light = dot(normalize(input.position.xyz), normalize(float3(1.0f, 1.0f, 0.0f)));
    // out.color = float4(abs(input.world_pos.xyz), 1.0f);
    // out.color ComputeLightRaymarch(input.world_pos, )
    out.color = ComputeForwardLightRaymarch(
        input.world_pos.xyz, normalize(input.world_pos.xyz - pc.camera_pos.xyz), pc, input);
    out.color.w = 1.0f;

    // out.color = abs(normalize(input.world_pos));
    return out;
}
