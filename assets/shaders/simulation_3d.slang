module simulation_3d;

__include simulation.spatial_hash_3d;
__include simulation.kernels_3d;

static const uint group_size = 256;

struct PushConstants {
    float time;
    float dt;
    uint n_particles;

    float3* positions;
    float3* velocities;
    float2* densities;
}

struct UniformConstants {
    float gravity;
    float damping_factor;
    float smoothing_radius;
    float target_density;
    float pressure_multiplier;
    float near_pressure_multiplier;
    float viscosity_strenght;

    float4 box;
    float4x4 world_to_local;
    float4x4 local_to_world;

    float spiky_pow3_scale;
    float spiky_pow2_scale;
    float spiky_pow3_diff_scale;
    float spiky_pow2_diff_scale;

    float3* predicted_positions;

    uint* spatial_keys;
    uint* spatial_offsets;
    uint* sorted_indices;

    float3* sort_target_positions;
    float3* sort_target_pred_positions;
    float3* sort_target_velocities;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformConstants> ubo;

float3 calc_external_forces(float3 pos, float3 vel) {
    let gravity_accel = float3(0, ubo.gravity, 0);
    return gravity_accel;
}

float PressureFromDensity(float density) {
    return (density - ubo.target_density) * ubo.pressure_multiplier;
}

float NearPressureFromDensity(float near_density) {
    return ubo.near_pressure_multiplier * near_density;
}

float2 calculate_density(float3 pos, uint n_particles) {
    let origin_cell = GetCell3D(pos, ubo.smoothing_radius);
    float sqr_radius = ubo.smoothing_radius * ubo.smoothing_radius;
    float density = 0.0f;
    float near_density = 0.0f;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, n_particles);
        uint curr_index = ubo.spatial_offsets[key];

        while (curr_index < n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            uint neighbor_key = ubo.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = ubo.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float dst = sqrt(sqr_dst_to_neighbor);
            density += DensityKernel(dst, ubo.smoothing_radius);
            near_density += NearDensityKernel(dst, ubo.smoothing_radius);
        }
    }

    return float2(density, near_density);
}

float3 calculate_pressure_force(uint id, PushConstants k) {
    float density = k.densities[id].x;
    float density_near = k.densities[id].y;
    float pressure = PressureFromDensity(density);
    float near_pressure = NearPressureFromDensity(density_near);
    var pressure_force = float3(0.0f);

    let velocity = k.velocities[id];
    var viscous_force = float3(0.0f);

    let pos = ubo.predicted_positions[id];
    let origin_cell = GetCell3D(pos, ubo.smoothing_radius);
    float sqr_radius = ubo.smoothing_radius * ubo.smoothing_radius;

    uint neighbor_count = 0;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = ubo.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            uint neighbor_key = ubo.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = ubo.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            neighbor_count++;

            let dens = k.densities[neighbor_index];
            float neighbor_density = dens.x;
            float neighbor_near_density = dens.y;
            float neighbor_pressure = PressureFromDensity(neighbor_density);
            float neighbor_near_pressure = NearPressureFromDensity(neighbor_near_density);

            float shared_pressure = (pressure + neighbor_pressure) * 0.5f;
            float shared_near_pressure = (near_pressure + neighbor_near_pressure) * 0.5f;

            float dst = sqrt(sqr_dst_to_neighbor);
            let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

            pressure_force += dir_to_neighbor * DensityDerivative(dst, ubo.smoothing_radius) *
                              shared_pressure / neighbor_density;
            pressure_force += dir_to_neighbor * NearDensityDerivative(dst, ubo.smoothing_radius) *
                              shared_near_pressure / neighbor_near_density;

            let neighbor_velocity = k.velocities[neighbor_index];
            viscous_force +=
                (neighbor_velocity - velocity) * SmoothingKernelPoly6(dst, ubo.smoothing_radius);
        }
    }

    return pressure_force / density;
}

void resolve_collisions(inout float3 pos, inout float3 vel) {
    float3 pos_local = mul(ubo.world_to_local, float4(pos, 1)).xyz;
    float3 vel_local = mul(ubo.world_to_local, float4(vel, 0)).xyz;

    const float3 half_size = 0.5;
    const float3 edge_dst = half_size - abs(pos_local);

    if (edge_dst.x <= 0) {
        pos_local.x = half_size.x * sign(pos_local.x);
        vel_local.x *= -1 * ubo.damping_factor;
    }
    if (edge_dst.y <= 0) {
        pos_local.y = half_size.y * sign(pos_local.y);
        vel_local.y *= -1 * ubo.damping_factor;
    }
    if (edge_dst.z <= 0) {
        pos_local.z = half_size.z * sign(pos_local.z);
        vel_local.z *= -1 * ubo.damping_factor;
    }

    pos = mul(ubo.local_to_world, float4(pos_local, 1)).xyz;
    vel = mul(ubo.local_to_world, float4(vel_local, 0)).xyz;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void external_forces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    k.velocities[id] += calc_external_forces(k.positions[id], k.velocities[id]) * k.dt;

    let prediction_factor = 1.0f / 120.0f;
    ubo.predicted_positions[id] = k.positions[id] + k.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void update_spatial_hash(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let cell = GetCell3D(ubo.predicted_positions[id], ubo.smoothing_radius);
    uint hash = HashCell3D(cell);
    uint key = KeyFromHash(hash, k.n_particles);
    ubo.spatial_keys[id] = key;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void reorder(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    uint sorted_index = ubo.sorted_indices[id];
    ubo.sort_target_positions[id] = k.positions[sorted_index];
    ubo.sort_target_pred_positions[id] = ubo.predicted_positions[sorted_index];
    ubo.sort_target_velocities[id] = k.velocities[sorted_index];
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void reorder_copyback(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    // This could be chanted to a set of VkCmdCopyBuffer calls
    k.positions[id] = ubo.sort_target_positions[id];
    ubo.predicted_positions[id] = ubo.sort_target_pred_positions[id];
    k.velocities[id] = ubo.sort_target_velocities[id];
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void calculate_densities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = ubo.predicted_positions[id];
    k.densities[id] = calculate_density(pos, k.n_particles);
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void calculate_pressure_forces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = calculate_pressure_force(id, k);
    k.velocities[id] += acc * k.dt;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void update_positions(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    var pos = k.positions[id];
    var vel = k.velocities[id];

    pos += vel * k.dt;
    resolve_collisions(pos, vel);

    k.positions[id] = pos;
    k.velocities[id] = vel;
}
