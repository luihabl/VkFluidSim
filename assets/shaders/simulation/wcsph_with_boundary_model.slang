module wcsph_model;
import common;
import spatial_hash.spatial_hash_3d;
import kernels.kernels_3d;

namespace wcsph_model {

    struct BoundaryObjInfo {
        float4x4 transform;
        float4x4 rotation;
        BoundingBox box;
    }

    struct Parameters {
        float wall_stiffness;
        float stiffness;
        float expoent;
        float viscosity_strenght;
        BoundaryObjInfo boundary_object;
    };

    [[vk::binding(n_global_bindings)]]
    ConstantBuffer<Parameters> parameters;
}

[[vk::binding(n_global_bindings + 1)]]
Sampler3D sdf_sampler;

[[vk::binding(n_global_bindings + 2)]]
Sampler3D volume_map_sampler;

float ParticleVolume() {
    return 0.8 * pow(simulation::parameters.smooth_radius, 3) / 8.0;
}

float3 CalculateExternalAccel(float3 pos, float3 vel) {
    let gravity_accel = simulation::parameters.gravity;
    return gravity_accel;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void ExternalAccel(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    sph_model::buffers.accelerations[id] =
        CalculateExternalAccel(sph_model::buffers.positions[id], sph_model::buffers.velocities[id]);
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
    let mass = sph_model::parameters.target_density * ParticleVolume();

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
            density += mass * kernel::CubicSpline(dst);
        }
    }

    // Add boundary density
    let box = wcsph_model.parameters.boundary_object.box;
    let local_pos = mul(wcsph_model.parameters.boundary_object.transform, float4(pos, 1.0)).xyz;
    let radius = simulation.parameters.smooth_radius;

    let inside = !any((local_pos < box.pos) || (local_pos > (box.pos + box.size)));

    float3 boundary_accel = 0;
    if (inside) {
        let uvw = ToUVW(box, local_pos);
        let dist = sdf_sampler.Sample(uvw).x;

        if (dist >= 0 && dist < radius) {
            let volume = volume_map_sampler.Sample(uvw).x;
            if (volume > 0) {
                var normal = GradientVector(local_pos, box);

                let modn = length(normal);

                if (modn > 1e-9) {
                    normal =
                        mul(wcsph_model.parameters.boundary_object.rotation, float4(normal, 1.0))
                            .xyz;
                    normal /= modn;

                    let d = dist;  // max(dist + 0.125 * radius, 0.5 * radius);
                    let x_boundary = (pos - d * normal);

                    let xij = pos - x_boundary;
                    let xij_mod = length(xij);
                    // let xij_norm = xij_mod > 0 ? xij / xij_mod : float3(0, 1, 0);

                    density += volume * sph_model::parameters.target_density *
                               kernel::CubicSpline(xij_mod);
                }
            }
        }
    }

    sph_model::buffers.densities[id] = density;
}

float PressureFromDensityEOS(float rho) {
    rho = max(rho, sph_model::parameters.target_density);
    return wcsph_model::parameters.stiffness *
           (pow(rho / sph_model::parameters.target_density, wcsph_model::parameters.expoent) - 1);
}

float3 PressureAccel(uint id, PushConstants k) {
    var pressure_force = float3(0.0f);

    let di = sph_model::buffers.densities[id];
    let pi = PressureFromDensityEOS(di);
    let pdi = di > 0 ? pi / (di * di) : 0.0f;

    let xi = sph_model::buffers.positions[id];
    let origin_cell = GetCell3D(xi, simulation::parameters.smooth_radius);
    let h2 = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;
    let mass = sph_model::parameters.target_density * ParticleVolume();

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
            let pj = PressureFromDensityEOS(dj);

            let xij_mod = sqrt(sqr_xij_mod);
            let xij_norm = xij_mod > 0 ? xij / xij_mod : float3(0, 1, 0);

            let pdj = dj > 0 ? pj / (dj * dj) : 0.0f;

            pressure_force += mass * (pdi + pdj) * kernel::GradCubicSpline(xij_mod) * xij_norm;
        }
    }

    return -pressure_force;
}

float3 ToUVW(BoundingBox box, float3 pos) {
    return (pos - box.pos) / box.size;
}

float3 GradientVector(float3 x, BoundingBox box) {
    let eps = 0.1 * simulation.parameters.smooth_radius;

    let dx = float3(eps, 0, 0);
    let dy = float3(0, eps, 0);
    let dz = float3(0, 0, eps);

    let v1x = sdf_sampler.Sample(ToUVW(box, x + dx)).x;
    let v0x = sdf_sampler.Sample(ToUVW(box, x - dx)).x;

    let v1y = sdf_sampler.Sample(ToUVW(box, x + dy)).x;
    let v0y = sdf_sampler.Sample(ToUVW(box, x - dy)).x;

    let v1z = sdf_sampler.Sample(ToUVW(box, x + dz)).x;
    let v0z = sdf_sampler.Sample(ToUVW(box, x - dz)).x;

    return float3(v1x - v0x, v1y - v0y, v1z - v0z) / (2.0 * eps);
}

float3 VolumeMapBoundaryAccel(uint id, PushConstants k) {
    let box = wcsph_model.parameters.boundary_object.box;
    let pos = sph_model.buffers.positions[id];
    let local_pos = mul(wcsph_model.parameters.boundary_object.transform, float4(pos, 1.0)).xyz;
    let radius = simulation.parameters.smooth_radius;

    let inside = !any((local_pos < box.pos) || (local_pos > (box.pos + box.size)));

    float3 boundary_accel = 0;
    if (inside) {
        let uvw = ToUVW(box, local_pos);
        let dist = sdf_sampler.Sample(uvw).x;

        if (dist >= 0 && dist < radius) {
            let volume = volume_map_sampler.Sample(uvw).x;

            if (volume > 0) {
                var normal = GradientVector(local_pos, box);

                let modn = length(normal);

                if (modn > 1e-9) {
                    normal =
                        mul(wcsph_model.parameters.boundary_object.rotation, float4(normal, 1.0))
                            .xyz;
                    normal /= modn;

                    let d = dist;  // max(dist + 0.125 * radius, 0.5 * radius);
                    let x_boundary = (pos - d * normal);

                    let xij = pos - x_boundary;
                    let xij_mod = length(xij);
                    let xij_norm = xij_mod > 0 ? xij / xij_mod : float3(0, 1, 0);

                    // sph_model.buffers.accelerations[id] += normal * 100.0;
                    // sph_model.buffers.velocities[id] = 0;
                    // sph_model.buffers.positions[id] = x_boundary;
                    let di = sph_model::buffers.densities[id];
                    let pi = PressureFromDensityEOS(di);
                    let pdi = di > 0 ? pi / (di * di) : 0.0f;
                    let pdj = di > 0 ? pi / (sph_model::parameters.target_density *
                                             sph_model::parameters.target_density)
                                     : 0.0f;

                    boundary_accel = -(pdi + pdj) * kernel::GradCubicSpline(xij_mod) * xij_norm *
                                     volume * sph_model::parameters.target_density;
                }
            }
        }
    }

    return boundary_accel;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculatePressureAccel(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    sph_model::buffers.accelerations[id] += PressureAccel(id, k);
    sph_model::buffers.accelerations[id] += VolumeMapBoundaryAccel(id, k);
}

float3 ViscousAccel(uint id, PushConstants k) {
    var viscous_force = float3(0.0f);

    let xi = sph_model::buffers.positions[id];
    let vi = sph_model::buffers.velocities[id];

    let origin_cell = GetCell3D(xi, simulation::parameters.smooth_radius);
    let h2 = simulation::parameters.smooth_radius * simulation::parameters.smooth_radius;

    let mass = sph_model::parameters.target_density * ParticleVolume();

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

            viscous_force += (mass / rhoj) * dot(vij, xij) * grad_w / (sqr_xij_mod + 0.01 * h2);
        }
    }

    static const uint dimension = 3;
    return 2.0f * (dimension + 2.0f) * wcsph_model::parameters.viscosity_strenght * viscous_force;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void CalculateViscousAccel(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    let acc = ViscousAccel(id, k);
    sph_model::buffers.accelerations[id] += acc;
}

float3 WallAcceleration(float3 pos, float3 vel) {
    let ll = sph_model::parameters.bounding_box.pos;
    let ur = ll + sph_model::parameters.bounding_box.size;

    let diff = min(pos - ll, float3(0.0)) - min(ur - pos, float3(0.0));

    if (abs(diff.x) > 1e-9 || abs(diff.y) > 1e-9 || abs(diff.z) > 1e-9) {
        return -wcsph_model::parameters.wall_stiffness * diff;
    }

    return float3(0.0);
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void UpdatePositions(uint id: SV_DispatchThreadID, uniform PushConstants k) {
    if (id >= k.n_particles)
        return;

    sph_model::buffers.accelerations[id] +=
        WallAcceleration(sph_model::buffers.positions[id], sph_model::buffers.velocities[id]);

    sph_model::buffers.velocities[id] += sph_model::buffers.accelerations[id] * k.dt;
    sph_model::buffers.positions[id] += sph_model::buffers.velocities[id] * k.dt;
}
