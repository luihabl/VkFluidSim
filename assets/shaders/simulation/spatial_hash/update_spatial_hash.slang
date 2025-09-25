import spatial_hash_3d;

struct UniformConstants {
    float cell_size;
    uint* spatial_keys;
}

[[vk::binding(0)]]
ConstantBuffer<UniformConstants> ubo;

struct PushConstants {
    float3* positions;
    uint n_particles;
}

static const uint group_size = 256;

[shader("compute")]
[numthreads(group_size, 1, 1)]
void update_spatial_hash(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let cell = GetCell3D(k.positions[id], ubo.cell_size);
    uint hash = HashCell3D(cell);
    uint key = KeyFromHash(hash, k.n_particles);
    ubo.spatial_keys[id] = key;
}
