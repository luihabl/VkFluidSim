module simulation_2d;

__include simulation.spatial_hash_2d;
__include simulation.kernels_2d;

static const uint group_size = 64;

struct PushConstants {
    float time;
    float dt;
    uint n_particles;

    float2* positions;
    float2* velocities;
    float2* densities;
}

struct BoundingBox {
    float2 size;
    float2 pos;
}

struct UniformConstants {
    float gravity;
    float damping_factor;
    float smoothing_radius;
    float target_density;
    float pressure_multiplier;
    float near_pressure_multiplier;
    float viscosity_strenght;

    BoundingBox box;

    float poly6_scale;
    float spiky_pow3_scale;
    float spiky_pow2_scale;
    float spiky_pow3_diff_scale;
    float spiky_pow2_diff_scale;

    float2* predicted_positions;

    uint* spatial_keys;
    uint* spatial_offsets;
    uint* sorted_indices;

    float2* sort_target_positions;
    float2* sort_target_pred_positions;
    float2* sort_target_velocities;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformConstants> ubo;

float2 CalculateExternalForces(float2 pos, float2 vel) {
    let gravity_accel = float2(0, ubo.gravity);
    return gravity_accel;
}

float PressureFromDensity(float density) {
    return (density - ubo.target_density) * ubo.pressure_multiplier;
}

float NearPressureFromDensity(float near_density) {
    return ubo.near_pressure_multiplier * near_density;
}

float2 CalculateDensity(float2 pos, uint n_particles) {
    let origin_cell = GetCell2D(pos, ubo.smoothing_radius);
    float sqr_radius = ubo.smoothing_radius * ubo.smoothing_radius;
    float density = 0.0f;
    float near_density = 0.0f;

    for (int i = 0; i < 9; i++) {
        uint hash = HashCell2D(origin_cell + offsets_2d[i]);
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

float2 CalculatePressureForce(uint id, PushConstants k) {
    float density = k.densities[id].x;
    float density_near = k.densities[id].y;
    let velocity = k.velocities[id];
    float pressure = PressureFromDensity(density);
    float near_pressure = NearPressureFromDensity(density_near);
    var pressure_force = float2(0.0f);
    var viscous_force = float2(0.0f);

    let pos = ubo.predicted_positions[id];
    let origin_cell = GetCell2D(pos, ubo.smoothing_radius);
    float sqr_radius = ubo.smoothing_radius * ubo.smoothing_radius;

    for (int i = 0; i < 9; i++) {
        uint hash = HashCell2D(origin_cell + offsets_2d[i]);
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

            float dst = sqrt(sqr_dst_to_neighbor);
            float2 dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float2(0, 1);

            const float2 dens = k.densities[neighbor_index];
            float neighbor_density = dens.x;
            float neighbor_near_density = dens.y;
            float neighbor_pressure = PressureFromDensity(neighbor_density);
            float neighbor_near_pressure = NearPressureFromDensity(neighbor_near_density);

            float shared_pressure = (pressure + neighbor_pressure) * 0.5f;
            float shared_near_pressure = (near_pressure + neighbor_near_pressure) * 0.5f;

            pressure_force += dir_to_neighbor * DensityDerivative(dst, ubo.smoothing_radius) *
                              shared_pressure / neighbor_density;
            pressure_force += dir_to_neighbor * NearDensityDerivative(dst, ubo.smoothing_radius) *
                              shared_near_pressure / neighbor_near_density;

            let neighbor_velocity = k.velocities[neighbor_index];
            viscous_force +=
                (neighbor_velocity - velocity) * ViscosityKernel(dst, ubo.smoothing_radius);
        }
    }

    return pressure_force / density + viscous_force * ubo.viscosity_strenght;
}

float2 calculate_viscosity_force(uint id, PushConstants k) {
    let pos = ubo.predicted_positions[id];
    let origin_cell = GetCell2D(pos, ubo.smoothing_radius);
    let sqr_radius = ubo.smoothing_radius * ubo.smoothing_radius;

    var viscosity_force = float2(0.0f);
    let velocity = k.velocities[id];

    for (int i = 0; i < 9; i++) {
        uint hash = HashCell2D(origin_cell + offsets_2d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = ubo.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            const uint neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            const uint neighbor_key = ubo.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = ubo.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float dst = sqrt(sqr_dst_to_neighbor);
            let neighbor_velocity = k.velocities[neighbor_index];
            viscosity_force +=
                (neighbor_velocity - velocity) * ViscosityKernel(dst, ubo.smoothing_radius);
        }
    }

    return viscosity_force;
}

void ProcessBoundaries(inout float2 p, inout float2 v) {
    if (p.x < ubo.box.pos.x) {
        p.x = ubo.box.pos.x;  // 2 * ubo.box.x - p.x;
        v.x = -v.x * ubo.damping_factor;
    }

    if (p.y < ubo.box.pos.y) {
        p.y = ubo.box.pos.y;  // 2*ubo.box.y-p.y;
        v.y = -v.y * ubo.damping_factor;
    }

    if (p.x > ubo.box.pos.x + ubo.box.size.x) {
        p.x = ubo.box.pos.x + ubo.box.size.x;  // 2 * (ubo.box.x + ubo.box.z) - p.x;
        v.x = -v.x * ubo.damping_factor;
    }

    if (p.y > ubo.box.pos.y + ubo.box.size.y) {
        p.y = ubo.box.pos.y + ubo.box.size.y;  // 2 * (ubo.box.y + ubo.box.w) - p.y;
        v.y = -v.y * ubo.damping_factor;
    }
}

float2 MoveParticle(float2 pos, float2 vel, float dt) {
    float2 p;
    p = pos + dt * vel;
    return p;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void ExternalForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    k.velocities[id] += CalculateExternalForces(k.positions[id], k.velocities[id]) * k.dt;

    let prediction_factor = 1.0f / 120.0f;
    ubo.predicted_positions[id] = k.positions[id] + k.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void update_spatial_hash(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let cell = GetCell2D(ubo.predicted_positions[id], ubo.smoothing_radius);
    uint hash = HashCell2D(cell);
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
void CalculateDensities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = ubo.predicted_positions[id];
    k.densities[id] = CalculateDensity(pos, k.n_particles);
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculatePressureForce(id, k);
    k.velocities[id] += acc * k.dt;
}

// [shader("compute")]
// [numthreads(group_size, 1, 1)]
// void calculate_viscosity_forces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
//     if (id >= k.n_particles)
//         return;

//     let viscosity_force = calculate_viscosity_force(id, k);
//     k.velocities[id] += viscosity_force * ubo.viscosity_strenght * k.dt;
// }

[shader("compute")]
[numthreads(group_size, 1, 1)]
void UpdatePositions(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    var pos = k.positions[id];
    var vel = k.velocities[id];

    var new_pos = MoveParticle(pos, vel, k.dt);
    ProcessBoundaries(new_pos, vel);

    k.positions[id] = new_pos;
    k.velocities[id] = vel;
}
