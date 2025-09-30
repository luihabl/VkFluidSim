#include "model.h"

#include <glm/ext/scalar_constants.hpp>
#include <random>

#include "gfx/common.h"
#include "imgui.h"
#include "simulation.h"

namespace vfs {
namespace {

std::vector<glm::vec3> SpawnParticlesInBox(const SPHModel::BoundingBox& box, u32 count) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;

    auto p = std::vector<glm::vec3>(count);

    for (u32 i = 0; i < count; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);
        float r3 = dist(rng);

        float x = box.pos.x + r1 * box.size.x;
        float y = box.pos.y + r2 * box.size.y;
        float z = box.pos.z + r3 * box.size.z;

        p[i] = glm::vec3{x, y, z};
    }

    return p;
}
}  // namespace

SPHModel::SPHModel(const Parameters* par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = Parameters{.time_scale = 1.0f,
                                .fixed_dt = 1.0f / 60.0f,
                                .iterations = 3,
                                .n_particles = 8192,
                                .target_density = 100.0f,
                                .bounding_box = {.pos{0.0f}, .size{10.0f}}};
    }
}

void SPHModel::Init(const gfx::CoreCtx& ctx) {
    spatial_hash.Init(ctx, parameters.n_particles,
                      Simulation::Get().GetGlobalParameters().smooth_radius);

    kernel_coeff_id = Simulation::Get().AddDescriptor(sizeof(KernelCoefficients));
    model_parameter_id = Simulation::Get().AddDescriptor(sizeof(Parameters));
    spatial_hash_buf_id = Simulation::Get().AddDescriptor(sizeof(SpatialHashBuffers));
    model_buffers_id = Simulation::Get().AddDescriptor(sizeof(ModelBuffers));

    SetBoundingBoxSize(parameters.bounding_box.size);
    buffers = CreateDataBuffers(ctx);

    AddBufferToBeReordered(buffers.position_buffer);
    AddBufferToBeReordered(buffers.velocity_buffer);
}

void SPHModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {}

void SPHModel::Clear(const gfx::CoreCtx& ctx) {
    spatial_hash.Clear(ctx);
    reorder.Clear(ctx);

    buffers.position_buffer.Destroy();
    buffers.velocity_buffer.Destroy();
    buffers.density_buffer.Destroy();

    pipeline.Clear(ctx);
}

void SPHModel::SetBoundingBoxSize(const glm::vec3& size) {
    bounding_box = BoundingBox{.size = size, .pos = {0.0f, 0.0f, 0.0f}};
}

void SPHModel::SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box) {
    auto pos = SpawnParticlesInBox(box, parameters.n_particles);
    auto vel = std::vector<glm::vec3>(parameters.n_particles, glm::vec3(0.0f));
    SetParticleState(gfx, pos, vel);
}

void SPHModel::SetParticleState(const gfx::Device& gfx,
                                const std::vector<glm::vec3>& pos,
                                const std::vector<glm::vec3>& vel) {
    gfx.SetDataVec(buffers.position_buffer, pos);
    gfx.SetDataVec(buffers.velocity_buffer, vel);
    gfx.SetDataVal(buffers.density_buffer, 0.0f);
}

SPHModel::DataBuffers SPHModel::CreateDataBuffers(const gfx::CoreCtx& ctx) const {
    return {
        .position_buffer = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles),
        .velocity_buffer = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles),
        .density_buffer = CreateDataBuffer<float>(ctx, SPHModel::parameters.n_particles),
    };
}

void SPHModel::CopyDataBuffers(VkCommandBuffer cmd, DataBuffers& dst) const {
#define CPY_BUFFER(name)                                                                         \
    {                                                                                            \
        auto cpy_info = VkBufferCopy{.dstOffset = 0, .srcOffset = 0, .size = buffers.name.size}; \
        vkCmdCopyBuffer(cmd, buffers.name.buffer, dst.name.buffer, 1, &cpy_info);                \
    }

    CPY_BUFFER(position_buffer);
    CPY_BUFFER(velocity_buffer);
    CPY_BUFFER(density_buffer);
}

void SPHModel::AddBufferToBeReordered(const gfx::Buffer& buffer) {
    reorder_buffers.push_back(buffer);
}

void SPHModel::InitBufferReorder(const gfx::CoreCtx& ctx) {
    reorder.Init(ctx, {
                          .buffers = reorder_buffers,
                          .sort_indices = spatial_hash.SpatialIndicesAddr(),
                          .n = (u32)parameters.n_particles,
                      });
}

void SPHModel::UpdateAllUniforms() {
    auto& sim = Simulation::Get();

    auto kernel_coeff =
        CalcKernelCoefficients(Simulation::Get().GetGlobalParameters().smooth_radius);
    sim.GetDescManager().SetUniformData(kernel_coeff_id, &kernel_coeff);
    sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);

    auto spatial_hash_bufs = SpatialHashBuffers{
        .spatial_keys = spatial_hash.SpatialKeysAddr(),
        .spatial_offsets = spatial_hash.SpatialOffsetsAddr(),
        .sorted_indices = spatial_hash.SpatialIndicesAddr(),
    };

    sim.GetDescManager().SetUniformData(spatial_hash_buf_id, &spatial_hash_bufs);

    auto model_bufs = ModelBuffers{
        .positions = buffers.position_buffer.device_addr,
        .velocities = buffers.velocity_buffer.device_addr,
        .densities = buffers.density_buffer.device_addr,
    };

    sim.GetDescManager().SetUniformData(model_buffers_id, &model_bufs);
}

void SPHModel::RunSpatialHash(VkCommandBuffer cmd,
                              const gfx::CoreCtx& ctx,
                              const gfx::Buffer* mod_positions) {
    if (mod_positions) {
        spatial_hash.Run(ctx, cmd, mod_positions->device_addr);
    } else {
        spatial_hash.Run(ctx, cmd, buffers.position_buffer.device_addr);
    }

    ComputeToComputePipelineBarrier(cmd);
    reorder.Reorder(cmd, ctx);
    ComputeToComputePipelineBarrier(cmd);
    reorder.Copyback(cmd);
}

SPHModel::KernelCoefficients SPHModel::CalcKernelCoefficients(float r) {
    return {
        .spiky_pow3_scale = 15.0f / (glm::pi<float>() * (float)std::pow(r, 6)),
        .spiky_pow2_scale = 15.0f / (2.0f * glm::pi<float>() * (float)std::pow(r, 5)),
        .spiky_pow3_diff_scale = 45.0f / (glm::pi<float>() * (float)std::pow(r, 6)),
        .spiky_pow2_diff_scale = 15.0f / (glm::pi<float>() * (float)std::pow(r, 5)),
    };
}

void SPHModel::DrawDebugUI() {
    auto& sim = Simulation::Get();

    if (ImGui::CollapsingHeader("Base model")) {
        if (ImGui::DragFloat("Time scale", &parameters.time_scale, 0.5f, 0.5f, 5.0f)) {
            sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);
        }

        if (ImGui::DragInt("Iterations", &parameters.iterations, 1, 1, 10)) {
            sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);
        }

        float dt = parameters.fixed_dt * 1e3;
        if (ImGui::DragFloat("Fixed delta-t", &dt, 1.0f, 1.0f, 20.0f, "%.2f ms")) {
            parameters.fixed_dt = dt * 1e-3;
            sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);
        }

        if (ImGui::SliderFloat("Target density", &parameters.target_density, 0.0f, 2000.0f)) {
            sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);
        }

        glm::vec3 size = parameters.bounding_box.size;
        if (ImGui::DragFloat3("Bounding box", &parameters.bounding_box.size.x, 0.1f, 0.5f, 2.0f)) {
            sim.GetDescManager().SetUniformData(model_parameter_id, &parameters);
        }
    }
}

}  // namespace vfs
