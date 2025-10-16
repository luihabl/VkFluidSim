#pragma once

#include "gfx/camera.h"
#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

class DensityRaymarchDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 transform;
        glm::mat4 view_proj;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format, VkFormat depth_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::MeshDrawObj& mesh,
              const gfx::Camera& camera);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

}  // namespace vfs
