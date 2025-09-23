#pragma once
#include <optional>

#include "gfx/common.h"
#include "pipeline.h"

namespace vfs {
class SPHModel {
public:
    struct BoundingBox {
        glm::vec3 size;
        glm::vec3 pos;
    };

    struct SimulationParameters {
        float time_scale;
        int iterations;
        int n_particles;
        float fixed_dt;
    };

    struct DataBuffers {
        gfx::Buffer position_buffer;
        gfx::Buffer velocity_buffer;
        gfx::Buffer density_buffer;
    };

    struct PushConstants {
        float time;
        float dt;
        unsigned n_particles;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
        VkDeviceAddress densities;
    };

    virtual void Init(const gfx::CoreCtx& ctx) = 0;
    virtual void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) = 0;
    virtual void Clear(const gfx::CoreCtx& ctx) = 0;
    virtual DataBuffers CreateDataBuffers(const gfx::CoreCtx& ctx) const = 0;
    virtual void DrawDebugUI() = 0;
    virtual ~SPHModel() = default;

    auto GetBoundingBox() const { return bounding_box; }
    const SimulationParameters& GetParameters() const { return parameters; }
    const DataBuffers& GetDataBuffers() const { return buffers; }
    void SetBoundingBoxSize(const glm::vec3& size);
    void SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box);
    void SetParticleState(const gfx::Device& gfx,
                          const std::vector<glm::vec3>& pos,
                          const std::vector<glm::vec3>& vel);
    void CopyDataBuffers(VkCommandBuffer cmd, DataBuffers& dst) const;

protected:
    DescriptorManager desc_manager;
    ComputePipeline pipeline;
    DataBuffers buffers;
    SimulationParameters parameters;
    std::optional<BoundingBox> bounding_box;

    u32 group_size{256};
};
}  // namespace vfs
