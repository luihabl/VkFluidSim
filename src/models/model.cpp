#include "model.h"

#include <glm/ext/scalar_constants.hpp>
#include <random>

#include "gfx/common.h"
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

    global_parameter_id = AddDescriptor(sizeof(SimulationParameters));
    kernel_coeff_id = AddDescriptor(sizeof(KernelCoefficients));
    model_parameter_id = AddDescriptor(sizeof(Parameters));
    spatial_hash_buf_id = AddDescriptor(sizeof(SpatialHashBuffers));

    SetBoundingBoxSize(parameters.bounding_box.size);
    buffers = CreateDataBuffers(ctx);
}

void SPHModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {}

void SPHModel::Clear(const gfx::CoreCtx& ctx) {
    spatial_hash.Clear(ctx);
    desc_manager.Clear(ctx);
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
    gfx.SetDataVal(buffers.density_buffer, glm::vec2(0.0f));
}

SPHModel::DataBuffers SPHModel::CreateDataBuffers(const gfx::CoreCtx& ctx) const {
    return {
        .position_buffer = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles),
        .velocity_buffer = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles),
        .density_buffer = CreateDataBuffer<glm::vec2>(ctx, SPHModel::parameters.n_particles),
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

u32 SPHModel::AddDescriptor(u32 size, gfx::DescriptorManager::DescType type) {
    descriptors.push_back({.size = size, .type = type});
    return descriptors.size() - 1;
}

void SPHModel::InitDescriptorManager(const gfx::CoreCtx& ctx) {
    desc_manager.Init(ctx, descriptors);
}

void SPHModel::UpdateAllUniforms() {
    desc_manager.SetUniformData(global_parameter_id, &Simulation::Get().GetGlobalParameters());
    auto kernel_coeff =
        CalcKernelCoefficients(Simulation::Get().GetGlobalParameters().smooth_radius);
    desc_manager.SetUniformData(kernel_coeff_id, &kernel_coeff);
    desc_manager.SetUniformData(model_parameter_id, &parameters);

    auto spatial_hash_bufs = SpatialHashBuffers{
        .spatial_keys = spatial_hash.SpatialKeysAddr(),
        .spatial_offsets = spatial_hash.SpatialOffsetsAddr(),
        .sorted_indices = spatial_hash.SpatialIndicesAddr(),
    };

    GetDescManager().SetUniformData(spatial_hash_buf_id, &spatial_hash_bufs);
}

SPHModel::KernelCoefficients SPHModel::CalcKernelCoefficients(float r) {
    return {
        .spiky_pow3_scale = 15.0f / (glm::pi<float>() * (float)std::pow(r, 6)),
        .spiky_pow2_scale = 15.0f / (2.0f * glm::pi<float>() * (float)std::pow(r, 5)),
        .spiky_pow3_diff_scale = 45.0f / (glm::pi<float>() * (float)std::pow(r, 6)),
        .spiky_pow2_diff_scale = 15.0f / (glm::pi<float>() * (float)std::pow(r, 5)),
    };
}

}  // namespace vfs
