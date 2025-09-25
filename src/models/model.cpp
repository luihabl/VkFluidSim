#include "model.h"

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
}

void SPHModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {}

void SPHModel::Clear(const gfx::CoreCtx& ctx) {
    spatial_hash.Clear(ctx);
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

}  // namespace vfs
