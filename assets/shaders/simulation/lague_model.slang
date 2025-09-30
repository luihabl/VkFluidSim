module lague_model;
import spatial_hash.spatial_hash_3d;
import kernels.kernels_3d;
import common;

namespace LagueModel {
    struct Parameters {
        float wall_damping_factor;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;
    };

    [[vk::binding(n_global_bindings)]]
    ConstantBuffer<LagueModel::Parameters> parameters;

    struct LagueModelBuffers {
        float3* predicted_positions;
        float* near_density;
    };

    [[vk::binding(n_global_bindings + 1)]]
    ConstantBuffer<LagueModelBuffers> buffers;
}

float DensityKernel(float dst, float radius) {
    return kernel::SpikyPow2(dst, radius);
}

float NearDensityKernel(float dst, float radius) {
    return kernel::SpikyPow3(dst, radius);
}

float DensityDerivative(float dst, float radius) {
    return kernel::GradSpikyPow2(dst, radius);
}

float NearDensityDerivative(float dst, float radius) {
    return kernel::GradSpikyPow3(dst, radius);
}

float3 CalculateExternalForces(float3 pos, float3 vel) {
    let gravity_accel = Simulation::parameters.gravity;
    return gravity_accel;
}

float PressureFromDensity(float density) {
    return (density - SPHModel::parameters.target_density) *
           LagueModel::parameters.pressure_multiplier;
}

float NearPressureFromDensity(float near_density) {
    return LagueModel::parameters.near_pressure_multiplier * near_density;
}

float2 CalculateDensity(float3 pos, uint n_particles) {
    let origin_cell = GetCell3D(pos, Simulation::parameters.smooth_radius);
    float sqr_radius = Simulation::parameters.smooth_radius * Simulation::parameters.smooth_radius;
    float density = 0.0f;
    float near_density = 0.0f;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, n_particles);
        uint curr_index = SPHModel::spatial_hash.spatial_offsets[key];

        while (curr_index < n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            uint neighbor_key = SPHModel::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = LagueModel::buffers.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float dst = sqrt(sqr_dst_to_neighbor);
            density += DensityKernel(dst, Simulation::parameters.smooth_radius);
            near_density += NearDensityKernel(dst, Simulation::parameters.smooth_radius);
        }
    }

    return float2(density, near_density);
}

float3 CalculatePressureForce(uint id, PushConstants k) {
    float density = SPHModel::buffers.densities[id];
    float density_near = LagueModel::buffers.near_density[id];
    float pressure = PressureFromDensity(density);
    float near_pressure = NearPressureFromDensity(density_near);
    var pressure_force = float3(0.0f);

    let velocity = SPHModel::buffers.velocities[id];
    var viscous_force = float3(0.0f);

    let pos = LagueModel::buffers.predicted_positions[id];
    let origin_cell = GetCell3D(pos, Simulation::parameters.smooth_radius);
    float sqr_radius = Simulation::parameters.smooth_radius * Simulation::parameters.smooth_radius;

    uint neighbor_count = 0;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = SPHModel::spatial_hash.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            uint neighbor_key = SPHModel::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = LagueModel::buffers.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            let sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            neighbor_count++;

            let neighbor_density = SPHModel::buffers.densities[neighbor_index];
            let neighbor_near_density = LagueModel::buffers.near_density[neighbor_index];
            let neighbor_pressure = PressureFromDensity(neighbor_density);
            let neighbor_near_pressure = NearPressureFromDensity(neighbor_near_density);

            let shared_pressure = (pressure + neighbor_pressure) * 0.5f;
            let shared_near_pressure = (near_pressure + neighbor_near_pressure) * 0.5f;

            let dst = sqrt(sqr_dst_to_neighbor);
            let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

            pressure_force += dir_to_neighbor *
                              DensityDerivative(dst, Simulation::parameters.smooth_radius) *
                              shared_pressure / neighbor_density;
            pressure_force += dir_to_neighbor *
                              NearDensityDerivative(dst, Simulation::parameters.smooth_radius) *
                              shared_near_pressure / neighbor_near_density;

            let neighbor_velocity = SPHModel::buffers.velocities[neighbor_index];
            viscous_force += (neighbor_velocity - velocity) *
                             kernel::Poly6(dst, Simulation::parameters.smooth_radius);
        }
    }

    return pressure_force / density + viscous_force * LagueModel::parameters.viscosity_strenght;
}

void ResolveCollisions(inout float3 pos, inout float3 vel) {
    let ll = SPHModel::parameters.bounding_box.pos;
    let ur = ll + SPHModel::parameters.bounding_box.size;

    if (pos.x < ll.x) {
        pos.x = ll.x;
        vel.x = -vel.x * LagueModel::parameters.wall_damping_factor;
    }

    if (pos.y < ll.y) {
        pos.y = ll.y;
        vel.y = -vel.y * LagueModel::parameters.wall_damping_factor;
    }

    if (pos.z < ll.z) {
        pos.z = ll.z;
        vel.z = -vel.z * LagueModel::parameters.wall_damping_factor;
    }

    if (pos.x > ur.x) {
        pos.x = ur.x;
        vel.x = -vel.x * LagueModel::parameters.wall_damping_factor;
    }

    if (pos.y > ur.y) {
        pos.y = ur.y;
        vel.y = -vel.y * LagueModel::parameters.wall_damping_factor;
    }

    if (pos.z > ur.z) {
        pos.z = ur.z;
        vel.z = -vel.z * LagueModel::parameters.wall_damping_factor;
    }
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void ExternalForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    SPHModel::buffers.velocities[id] +=
        CalculateExternalForces(SPHModel::buffers.positions[id], SPHModel::buffers.velocities[id]) *
        k.dt;

    let prediction_factor = 1.0f / 120.0f;
    LagueModel::buffers.predicted_positions[id] =
        SPHModel::buffers.positions[id] + SPHModel::buffers.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateDensities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = LagueModel::buffers.predicted_positions[id];
    let d = CalculateDensity(pos, k.n_particles);
    SPHModel::buffers.densities[id] = d.x;
    LagueModel::buffers.near_density[id] = d.y;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculatePressureForce(id, k);
    SPHModel::buffers.velocities[id] += acc * k.dt;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void UpdatePositions(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    var pos = SPHModel::buffers.positions[id];
    var vel = SPHModel::buffers.velocities[id];

    pos += vel * k.dt;

    // TODO: Remove this method, use a particle-based representation (read Green thesis to
    // understand the different types of boundaries). Try to find an example of implementation of
    // particle-based boundary.
    ResolveCollisions(pos, vel);

    SPHModel::buffers.positions[id] = pos;
    SPHModel::buffers.velocities[id] = vel;
}
