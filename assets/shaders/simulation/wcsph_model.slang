module wcsph_model;
import common;
import spatial_hash.spatial_hash_3d;
import kernels.kernels_3d;

namespace wcsph_model {

    struct Parameters {
        float wall_damping_factor;
        float stiffness;
        float expoent;
        float viscosity_strenght;
    };

    [[vk::binding(n_global_bindings)]]
    ConstantBuffer<Parameters> parameters;
}

float3 CalculateExternalForces(float3 pos, float3 vel) {
    let gravity_accel = simulation::parameters.gravity;
    return gravity_accel;
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

    // let prediction_factor = 1.0f / 120.0f;
    // buffers.predicted_positions[id] =
    //     k.positions[id] + k.velocities[id] * prediction_factor;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateDensities(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let pos = sph_model::buffers.positions[id];

    let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
    float sqr_radius = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;
    float density = 0.0f;

    for (int i = 0; i < 27; i++) {
        uint hash = HashCell3D(origin_cell + offsets_3d[i]);
        uint key = KeyFromHash(hash, k.n_particles);
        uint curr_index = sph_model::spatial_hash.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            uint neighbor_index = curr_index;
            curr_index++;

            uint neighbor_key = sph_model::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = sph_model::buffers.positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float dst = sqrt(sqr_dst_to_neighbor);
            density += kernel::CubicSpline(dst);
        }
    }

    sph_model::buffers.densities[id] = density;
}

float pressure_from_density_eos(float rho) {
    return wcsph_model::parameters.stiffness *
           (pow(rho / sph_model::parameters.target_density, wcsph_model::parameters.expoent) - 1);
}

float3 CalculatePressureForce(uint id, PushConstants k) {
    float density = sph_model::buffers.densities[id];
    float pressure = pressure_from_density_eos(density);
    var pressure_force = float3(0.0f);

    let pos = sph_model::buffers.positions[id];
    let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
    float sqr_radius = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

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

            let neighbor_pos = sph_model::buffers.positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float neighbor_density = sph_model::buffers.densities[neighbor_index].x;
            float neighbor_pressure = pressure_from_density_eos(neighbor_density);

            // TODO: Check if this is necessary
            float shared_pressure = (pressure + neighbor_pressure) * 0.5f;

            float dst = sqrt(sqr_dst_to_neighbor);
            let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

            pressure_force +=
                dir_to_neighbor * kernel::GradCubicSpline(dst) * shared_pressure / neighbor_density;
        }
    }

    return pressure_force / density;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculatePressureForce(id, k);
    sph_model::buffers.velocities[id] += acc * k.dt;
}

// float3 calculate_viscous_force(uint id, PushConstants k) {
//     float density = sph_model::buffers.densities[id];
//     float pressure = pressure_from_density_eos(density);
//     // var pressure_force = float3(0.0f);

//     let pos = sph_model::buffers.positions[id];
//     let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
//     float sqr_radius = simulation::parameters.smooth_radius *
//     simulation::parameters.smooth_radius;

//     for (int i = 0; i < 27; i++) {
//         uint hash = HashCell3D(origin_cell + offsets_3d[i]);
//         uint key = KeyFromHash(hash, k.n_particles);
//         uint curr_index = sph_model::spatial_hash.spatial_offsets[key];

//         while (curr_index < k.n_particles) {
//             uint neighbor_index = curr_index;
//             curr_index++;

//             if (neighbor_index == id)
//                 continue;

//             uint neighbor_key = sph_model::spatial_hash.spatial_keys[neighbor_index];

//             if (neighbor_key != key)
//                 break;

//             let neighbor_pos = sph_model::buffers.positions[neighbor_index];
//             let offset_to_neighbor = neighbor_pos - pos;
//             float sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

//             if (sqr_dst_to_neighbor > sqr_radius)
//                 continue;

//             float neighbor_density = sph_model::buffers.densities[neighbor_index].x;
//             float neighbor_pressure = pressure_from_density_eos(neighbor_density);

//             // TODO: Check if this is necessary
//             float shared_pressure = (pressure + neighbor_pressure) * 0.5f;

//             float dst = sqrt(sqr_dst_to_neighbor);
//             let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

//             pressure_force += dir_to_neighbor *
//                               DensityDerivative(dst, simulation::parameters.smooth_radius) *
//                               shared_pressure / neighbor_density;
//         }
//     }

//     return pressure_force / density;
//     sph_model::buffers.velocities[id] += acc * k.dt;
// }

float3 CalculateViscousForce(uint id, PushConstants k) {
    let velocity = sph_model::buffers.velocities[id];
    var viscous_force = float3(0.0f);

    let pos = sph_model::buffers.positions[id];
    let origin_cell = GetCell3D(pos, simulation::parameters.smooth_radius);
    let sqr_radius = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

    for (int i = 0; i < 27; i++) {
        let hash = HashCell3D(origin_cell + offsets_3d[i]);
        let key = KeyFromHash(hash, k.n_particles);
        var curr_index = sph_model::spatial_hash.spatial_offsets[key];

        while (curr_index < k.n_particles) {
            let neighbor_index = curr_index;
            curr_index++;

            if (neighbor_index == id)
                continue;

            let neighbor_key = sph_model::spatial_hash.spatial_keys[neighbor_index];

            if (neighbor_key != key)
                break;

            let neighbor_pos = sph_model::buffers.positions[neighbor_index];
            let offset_to_neighbor = neighbor_pos - pos;
            let sqr_dst_to_neighbor = dot(offset_to_neighbor, offset_to_neighbor);

            if (sqr_dst_to_neighbor > sqr_radius)
                continue;

            float neighbor_density = sph_model::buffers.densities[neighbor_index];
            // float neighbor_pressure = pressure_from_density_eos(neighbor_density);

            // TODO: Check if this is necessary
            // float shared_pressure = (pressure + neighbor_pressure) * 0.5f;

            float dst = sqrt(sqr_dst_to_neighbor);
            let dir_to_neighbor = dst > 0 ? offset_to_neighbor / dst : float3(0, 1, 0);

            let neighbor_velocity = sph_model::buffers.velocities[neighbor_index];
            let v_relative = neighbor_velocity - velocity;

            viscous_force += dir_to_neighbor * dot(v_relative, offset_to_neighbor) *
                             kernel::GradCubicSpline(dst) / neighbor_density /
                             (sqr_dst_to_neighbor + 0.01 * sqr_radius);
        }
    }

    return viscous_force * wcsph_model::parameters.viscosity_strenght;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateViscousForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculateViscousForce(id, k);
    sph_model::buffers.velocities[id] += acc * k.dt;
}

void ResolveCollisions(inout float3 pos, inout float3 vel) {
    let ll = sph_model::parameters.bounding_box.pos;
    let ur = ll + sph_model::parameters.bounding_box.size;

    if (pos.x < ll.x) {
        pos.x = ll.x;
        vel.x = -vel.x * wcsph_model::parameters.wall_damping_factor;
    }

    if (pos.y < ll.y) {
        pos.y = ll.y;
        vel.y = -vel.y * wcsph_model::parameters.wall_damping_factor;
    }

    if (pos.z < ll.z) {
        pos.z = ll.z;
        vel.z = -vel.z * wcsph_model::parameters.wall_damping_factor;
    }

    if (pos.x > ur.x) {
        pos.x = ur.x;
        vel.x = -vel.x * wcsph_model::parameters.wall_damping_factor;
    }

    if (pos.y > ur.y) {
        pos.y = ur.y;
        vel.y = -vel.y * wcsph_model::parameters.wall_damping_factor;
    }

    if (pos.z > ur.z) {
        pos.z = ur.z;
        vel.z = -vel.z * wcsph_model::parameters.wall_damping_factor;
    }
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
