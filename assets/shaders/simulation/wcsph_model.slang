module wcsph_model;
import common;
import spatial_hash.spatial_hash_3d;
import kernels.kernels_3d;

namespace wcsph_model {

    struct Parameters {
        float wall_stiffness;
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
    rho = max(rho, sph_model::parameters.target_density);
    return wcsph_model::parameters.stiffness *
           (pow(rho / sph_model::parameters.target_density, wcsph_model::parameters.expoent) - 1);
}

float3 CalculatePressureForce(uint id, PushConstants k) {
    var pressure_force = float3(0.0f);

    let di = sph_model::buffers.densities[id];
    let pi = pressure_from_density_eos(di);
    let pdi = di > 0 ? pi / (di * di) : 0.0f;

    let xi = sph_model::buffers.positions[id];
    let origin_cell = GetCell3D(xi, simulation::parameters.smooth_radius);
    let h2 = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

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

            let xj = sph_model::buffers.positions[neighbor_index];
            let xij = xi - xj;
            let sqr_xij_mod = dot(xij, xij);

            if (sqr_xij_mod > h2)
                continue;

            let dj = sph_model::buffers.densities[neighbor_index];
            let pj = pressure_from_density_eos(dj);

            let xij_mod = sqrt(sqr_xij_mod);
            let xij_norm = xij_mod > 0 ? xij / xij_mod : float3(0, 1, 0);

            let pdj = dj > 0 ? pj / (dj * dj) : 0.0f;

            pressure_force += (pdi + pdj) * kernel::GradCubicSpline(xij_mod) * xij_norm;
        }
    }

    return -pressure_force;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculatePressureForce(id, k);
    sph_model::buffers.velocities[id] += acc * k.dt;
}

float3 CalculateViscousForce(uint id, PushConstants k) {
    var viscous_force = float3(0.0f);

    let xi = sph_model::buffers.positions[id];
    let vi = sph_model::buffers.velocities[id];

    let origin_cell = GetCell3D(xi, simulation::parameters.smooth_radius);
    let h2 = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

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

            let xj = sph_model::buffers.positions[neighbor_index];
            let xij = xi - xj;
            let sqr_xij_mod = dot(xij, xij);

            if (sqr_xij_mod > h2)
                continue;

            let xij_mod = sqrt(sqr_xij_mod);
            let xij_norm = xij_mod > 0 ? xij / xij_mod : float3(0, 1, 0);

            let vj = sph_model::buffers.velocities[neighbor_index];
            let vij = vi - vj;

            let rhoj = max(sph_model::buffers.densities[neighbor_index], 1e-6);

            let grad_w = xij_norm * kernel::GradCubicSpline(xij_mod);

            viscous_force += (1.0f / rhoj) * dot(vij, xij) * grad_w / (sqr_xij_mod + 0.01 * h2);
        }
    }

    static const uint dimension = 3;
    return 2.0f * (dimension + 2.0f) * wcsph_model::parameters.viscosity_strenght * viscous_force;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateViscousForces(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = CalculateViscousForce(id, k);
    sph_model::buffers.velocities[id] += acc * k.dt;
}

void ResolveCollisions(inout float3 pos, inout float3 vel, float dt) {
    let ll = sph_model::parameters.bounding_box.pos;
    let ur = ll + sph_model::parameters.bounding_box.size;

    let diff = min(pos - ll, float3(0.0)) - min(ur - pos, float3(0.0));

    if (abs(diff.x) > 1e-9 || abs(diff.y) > 1e-9 || abs(diff.z) > 1e-9) {
        vel = vel - wcsph_model::parameters.wall_stiffness * diff * dt;
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
    ResolveCollisions(pos, vel, k.dt);

    sph_model::buffers.positions[id] = pos;
    sph_model::buffers.velocities[id] = vel;
}
