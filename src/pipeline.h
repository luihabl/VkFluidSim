#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {
class SpriteDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        VkDeviceAddress vertex_buffer;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clean(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};
}  // namespace vfs