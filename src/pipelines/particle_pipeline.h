#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

class ParticleDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const PushConstants& push_constants,
              uint32_t instances);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class Particle3DDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 model_view;
        glm::mat4 proj;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format, VkFormat depth_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const PushConstants& push_constants,
              uint32_t instances);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

}  // namespace vfs
