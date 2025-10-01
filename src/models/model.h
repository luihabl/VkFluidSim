#pragma once
#include <optional>

#include "compute/compute_pipeline.h"
#include "compute/sort.h"
#include "compute/spatial_hash.h"
#include "gfx/common.h"
#include "gfx/gfx.h"

namespace vfs {
class SPHModel {
public:
    struct BoundingBox {
        glm::vec3 size;
        glm::vec3 pos;
    };

    struct Parameters {
        float time_scale;
        int iterations;
        int n_particles;
        float fixed_dt;
        float target_density;
        BoundingBox bounding_box;
    };

    struct KernelCoefficients {
        float spiky_pow3_scale;
        float spiky_pow2_scale;
        float spiky_pow3_diff_scale;
        float spiky_pow2_diff_scale;
        float cubic_spline_scale;
        float grad_cubic_spline_scale;
    };

    struct SpatialHashBuffers {
        VkDeviceAddress spatial_keys;
        VkDeviceAddress spatial_offsets;
        VkDeviceAddress sorted_indices;
    };

    struct DataBuffers {
        gfx::Buffer position_buffer;
        gfx::Buffer velocity_buffer;
        gfx::Buffer density_buffer;
    };

    struct ModelBuffers {
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
        VkDeviceAddress densities;
    };

    struct PushConstants {
        float time;
        float dt;
        unsigned n_particles;
    };

    SPHModel(const Parameters* parameters = nullptr);

    virtual void Init(const gfx::CoreCtx& ctx);
    virtual void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd);
    virtual void Clear(const gfx::CoreCtx& ctx);
    virtual void DrawDebugUI();
    virtual ~SPHModel() = default;

    auto GetBoundingBox() const { return bounding_box; }
    const Parameters& GetParameters() const { return parameters; }
    const DataBuffers& GetDataBuffers() const { return buffers; }

    void SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box);
    void SetParticleState(const gfx::Device& gfx,
                          const std::vector<glm::vec3>& pos,
                          const std::vector<glm::vec3>& vel);
    void SetBoundingBoxSize(const glm::vec3& size);

    void CopyDataBuffers(VkCommandBuffer cmd, DataBuffers& dst) const;
    DataBuffers CreateDataBuffers(const gfx::CoreCtx& ctx) const;

protected:
    ComputePipeline pipeline;
    DataBuffers buffers;
    Parameters parameters;
    std::optional<BoundingBox> bounding_box;
    SpatialHash spatial_hash;
    u32 group_size{256};

    u32 kernel_coeff_id;
    u32 model_parameter_id;
    u32 spatial_hash_buf_id;
    u32 model_buffers_id;

    void UpdateAllUniforms();

    void AddBufferToBeReordered(const gfx::Buffer& buffer);
    void InitBufferReorder(const gfx::CoreCtx& ctx);

    void RunSpatialHash(VkCommandBuffer cmd,
                        const gfx::CoreCtx& ctx,
                        const gfx::Buffer* mod_positions = nullptr);

    KernelCoefficients CalcKernelCoefficients(float r);

private:
    BufferReorder reorder;
    std::vector<BufferReorder::Config::BufferInfo> reorder_buffers;
};
}  // namespace vfs
