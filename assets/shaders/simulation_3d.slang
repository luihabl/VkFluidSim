module simulation_3d;
import simulation.spatial_hash_3d;
import simulation.kernels_3d;

static const uint group_size = 256;

struct PushConstants {
    float time;
    float dt;
    uint n_particles;

    float3* positions;
    float3* velocities;
    float2* densities;
}

struct BoundingBox {
    float3 size;
    float3 pos;
}

namespace Simulation {
    struct SimulationParameters {
        float3 gravity;
        float smooth_radius;
    };

    [[vk::binding(0)]]
    ConstantBuffer<SimulationParameters> parameters;
}

namespace SPHModel {

    // struct KernelCoefficients {
    //     float spiky_pow3_scale;
    //     float spiky_pow2_scale;
    //     float spiky_pow3_diff_scale;
    //     float spiky_pow2_diff_scale;
    // };

    // [[vk::binding(1)]]
    // internal ConstantBuffer<KernelCoefficients> kernel_coeff;

    struct Parameters {
        float time_scale;
        int iterations;
        int n_particles;
        float fixed_dt;
        float target_density;
        BoundingBox bounding_box;
    };

    [[vk::binding(2)]]
    ConstantBuffer<SPHModel::Parameters> parameters;
}

namespace LagueModel {
    struct Parameters {
        float wall_damping_factor;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;
    };

    [[vk::binding(3)]]
    ConstantBuffer<LagueModel::Parameters> parameters;

    struct SpatialHashBuffers {
        uint* spatial_keys;
        uint* spatial_offsets;
        uint* sorted_indices;
    };

    [[vk::binding(4)]]
    ConstantBuffer<SpatialHashBuffers> spatial_hash;

    struct LagueModelBuffers {
        float3* predicted_positions;
        float3* sort_target_positions;
        float3* sort_target_pred_positions;
        float3* sort_target_velocities;
    };

    [[vk::binding(5)]]
    ConstantBuffer<LagueModelBuffers> buffers;
}

float3 calc_external_forces(float3 pos, float3 vel) {
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

float2 calculate_density(float3 pos, uint n_particles) {
    let origin_cell = GetCell3D(pos, Simulation::parameters.smooth_radius);
    float sqr_radius = Simulation::parameters.smooth_radius * Simulation::parameters.smooth_radius;
    float density = 0.0f;
    float near_density = 0.0f;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, n_particles);
        uint curr_index = LagueModel::spatial_hash.spatial_offsets[key];

        while (curr_index < n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            uint neighbor_key = LagueModel::spatial_hash.spatial_keys[neighbor_index];

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

float3 calculate_pressure_force(uint id, PushConstants k) {
    float density = k.densities[id].x;
    float density_near = k.densities[id].y;
    float pressure = PressureFromDensity(density);
    float near_pressure = NearPressureFromDensity(density_near);
    var pressure_force = float3(0.0f);

    let velocity = k.velocities[id];
    var viscous_force = float3(0.0f);

    let pos = LagueModel::buffers.predicted_positions[id];
    let origin_cell = GetCell3D(pos, Simulation::parameters.smooth_radius);
    float sqr_radius = Simulation::parameters.smooth_radius * Simulation::parameters.smooth_radius;

    uint neighbor_count = 0;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = LagueModel::spatial_hash.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            uint neighbor_key = LagueModel::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = LagueModel::buffers.predicted_positions[neighbor_index];
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

            pressure_force += dir_to_neighbor *
                              DensityDerivative(dst, Simulation::parameters.smooth_radius) *
                              shared_pressure / neighbor_density;
            pressure_force += dir_to_neighbor *
                              NearDensityDerivative(dst, Simulation::parameters.smooth_radius) *
                              shared_near_pressure / neighbor_near_density;

            let neighbor_velocity = k.velocities[neighbor_index];
            viscous_force += (neighbor_velocity - velocity) *
                             SmoothingKernelPoly6(dst, Simulation::parameters.smooth_radius);
        }
    }

    return pressure_force / density + viscous_force * LagueModel::parameters.viscosity_strenght;
}

void resolve_collisions(inout float3 pos, inout float3 vel) {
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
void external_forces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    k.velocities[id] += calc_external_forces(k.positions[id], k.velocities[id]) * k.dt;

    let prediction_factor = 1.0f / 120.0f;
    LagueModel::buffers.predicted_positions[id] =
        k.positions[id] + k.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void reorder(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    uint sorted_index = LagueModel::spatial_hash.sorted_indices[id];
    LagueModel::buffers.sort_target_positions[id] = k.positions[sorted_index];
    LagueModel::buffers.sort_target_pred_positions[id] =
        LagueModel::buffers.predicted_positions[sorted_index];
    LagueModel::buffers.sort_target_velocities[id] = k.velocities[sorted_index];
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void reorder_copyback(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    // This could be chanted to a set of VkCmdCopyBuffer calls
    k.positions[id] = LagueModel::buffers.sort_target_positions[id];
    LagueModel::buffers.predicted_positions[id] =
        LagueModel::buffers.sort_target_pred_positions[id];
    k.velocities[id] = LagueModel::buffers.sort_target_velocities[id];
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void calculate_densities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = LagueModel::buffers.predicted_positions[id];
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

    // TODO: Remove this method, use a particle-based representation (read Green thesis to
    // understand the different types of boundaries). Try to find an example of implementation of
    // particle-based boundary.
    resolve_collisions(pos, vel);

    k.positions[id] = pos;
    k.velocities[id] = vel;
}
