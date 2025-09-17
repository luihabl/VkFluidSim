
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

// Draw 2D box
static const float2 coords_2d[] = {
    float2(0, 0), float2(1, 0), float2(1, 0), float2(1, 1),
    float2(1, 1), float2(0, 1), float2(0, 1), float2(0, 0),
};

[shader("vertex")]
VertexOutput vertex_main(uint vertex_id: SV_VertexID, uniform PushConstants pc) {
    VertexOutput out;

    out.position = mul(pc.matrix, float4(coords_2d[vertex_id], 0.0f, 1.0f));
    out.color = pc.color;
    return out;
}

[shader("fragment")]
FragmentOutput frag_main(VertexOutput input) {
    FragmentOutput out;
    out.color = input.color;
    return out;
}

// Draw 3D box
static const float3 coords_3d[] = {
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
    { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 },
};

static const uint2 edges[] = {
    // front face
    { 0, 1 },
    { 1, 2 },
    { 2, 3 },
    { 3, 0 },

    // back side
    { 4, 5 },
    { 5, 6 },
    { 6, 7 },
    { 7, 4 },

    // side edges
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 },
};

[shader("vertex")]
VertexOutput vertex_3d_main(uint vertex_id: SV_VertexID, uniform PushConstants pc) {
    VertexOutput out;

    uint n_edge = vertex_id / 2;
    let index = vertex_id % 2 == 0 ? edges[n_edge].x : edges[n_edge].y;

    let coord = coords_3d[index];
    out.position = mul(pc.matrix, float4(coord, 1.0f));
    out.color = pc.color * (1 - coord.z * 0.9);
    return out;
}

[shader("fragment")]
FragmentOutput frag_3d_main(VertexOutput input) {
    FragmentOutput out;
    out.color = input.color;
    return out;
}
