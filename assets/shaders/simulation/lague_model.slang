module lague_model;
import spatial_hash.spatial_hash_3d;
import kernels.kernels_3d;
import common;

namespace lague_model {
    struct Parameters {
        float wall_damping_factor;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;
    };

    [[vk::binding(n_global_bindings)]]
    ConstantBuffer<lague_model::Parameters> parameters;

    struct LagueModelBuffers {
        float3* predicted_positions;
        float* near_density;
    };

    [[vk::binding(n_global_bindings + 1)]]
    ConstantBuffer<LagueModelBuffers> buffers;
}

float DensityKernel(float dst) {
    return kernel::SpikyPow2(dst);
}

float NearDensityKernel(float dst) {
    return kernel::SpikyPow3(dst);
}

float DensityDerivative(float dst) {
    return kernel::GradSpikyPow2(dst);
}

float NearDensityDerivative(float dst) {
    return kernel::GradSpikyPow3(dst);
}

float3 CalculateExternalForces(float3 pos, float3 vel) {
    let gravity_accel = simulation::parameters.gravity;
    return gravity_accel;
}

float PressureFromDensity(float density) {
    return (density - sph_model::parameters.target_density) *
           lague_model::parameters.pressure_multiplier;
}

float NearPressureFromDensity(float near_density) {
    return lague_model::parameters.near_pressure_multiplier * near_density;
}

float2 CalculateDensity(float3 pos, uint n_particles) {
    let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
    float sqr_radius = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;
    float density = 0.0f;
    float near_density = 0.0f;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, n_particles);
        uint curr_index = sph_model::spatial_hash.spatial_offsets[key];

        while (curr_index < n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            uint neighbor_key = sph_model::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = lague_model::buffers.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float dst = sqrt(sqr_dst_to_neighbor);
            density += DensityKernel(dst);
            near_density += NearDensityKernel(dst);
        }
    }

    return float2(density, near_density);
}

float3 CalculatePressureForce(uint id, PushConstants k) {
    float density = sph_model::buffers.densities[id];
    float density_near = lague_model::buffers.near_density[id];
    float pressure = PressureFromDensity(density);
    float near_pressure = NearPressureFromDensity(density_near);
    var pressure_force = float3(0.0f);

    let velocity = sph_model::buffers.velocities[id];
    var viscous_force = float3(0.0f);

    let pos = lague_model::buffers.predicted_positions[id];
    let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
    float sqr_radius = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

    uint neighbor_count = 0;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = sph_model::spatial_hash.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            uint neighbor_key = sph_model::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = lague_model::buffers.predicted_positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            let sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            neighbor_count++;

            let neighbor_density = sph_model::buffers.densities[neighbor_index];
            let neighbor_near_density = lague_model::buffers.near_density[neighbor_index];
            let neighbor_pressure = PressureFromDensity(neighbor_density);
            let neighbor_near_pressure = NearPressureFromDensity(neighbor_near_density);

            let shared_pressure = (pressure + neighbor_pressure) * 0.5f;
            let shared_near_pressure = (near_pressure + neighbor_near_pressure) * 0.5f;

            let dst = sqrt(sqr_dst_to_neighbor);
            let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

            pressure_force +=
                dir_to_neighbor * DensityDerivative(dst) * shared_pressure / neighbor_density;
            pressure_force += dir_to_neighbor * NearDensityDerivative(dst) * shared_near_pressure /
                              neighbor_near_density;

            let neighbor_velocity = sph_model::buffers.velocities[neighbor_index];
            viscous_force += (neighbor_velocity - velocity) * kernel::Poly6(dst);
        }
    }

    return pressure_force / density + viscous_force * lague_model::parameters.viscosity_strenght;
}

void ResolveCollisions(inout float3 pos, inout float3 vel) {
    let ll = sph_model::parameters.bounding_box.pos;
    let ur = ll + sph_model::parameters.bounding_box.size;

    if (pos.x < ll.x) {
        pos.x = ll.x;
        vel.x = -vel.x * lague_model::parameters.wall_damping_factor;
    }

    if (pos.y < ll.y) {
        pos.y = ll.y;
        vel.y = -vel.y * lague_model::parameters.wall_damping_factor;
    }

    if (pos.z < ll.z) {
        pos.z = ll.z;
        vel.z = -vel.z * lague_model::parameters.wall_damping_factor;
    }

    if (pos.x > ur.x) {
        pos.x = ur.x;
        vel.x = -vel.x * lague_model::parameters.wall_damping_factor;
    }

    if (pos.y > ur.y) {
        pos.y = ur.y;
        vel.y = -vel.y * lague_model::parameters.wall_damping_factor;
    }

    if (pos.z > ur.z) {
        pos.z = ur.z;
        vel.z = -vel.z * lague_model::parameters.wall_damping_factor;
    }
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void ExternalForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    sph_model::buffers.velocities[id] +=
        CalculateExternalForces(sph_model::buffers.positions[id],
                                sph_model::buffers.velocities[id]) *
        k.dt;

    let prediction_factor = 1.0f / 120.0f;
    lague_model::buffers.predicted_positions[id] =
        sph_model::buffers.positions[id] + sph_model::buffers.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateDensities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = lague_model::buffers.predicted_positions[id];
    let d = CalculateDensity(pos, k.n_particles);
    sph_model::buffers.densities[id] = d.x;
    lague_model::buffers.near_density[id] = d.y;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculatePressureForce(id, k);
    sph_model::buffers.velocities[id] += acc * k.dt;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void UpdatePositions(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    var pos = sph_model::buffers.positions[id];
    var vel = sph_model::buffers.velocities[id];

    pos += vel * k.dt;

    // TODO: Remove this method, use a particle-based representation (read Green thesis to
    // understand the different types of boundaries). Try to find an example of implementation of
    // particle-based boundary.
    ResolveCollisions(pos, vel);

    sph_model::buffers.positions[id] = pos;
    sph_model::buffers.velocities[id] = vel;
}
