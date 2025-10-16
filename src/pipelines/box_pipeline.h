#pragma once
#include "gfx/common.h"
#include "gfx/gfx.h"

namespace vfs {

class BoxDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        glm::vec4 color{1.0f};
    };

    void Init(const gfx::CoreCtx& ctx,
              VkFormat draw_img_format,
              VkFormat depth_img_format,
              bool draw_3d = false);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const PushConstants& push_constants);

private:
    u32 n_draw_vertices{8};
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

}  // namespace vfs
