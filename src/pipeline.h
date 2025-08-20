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
        VkDeviceAddress pos_buffer;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clean(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const gfx::Buffer& buf,
              const glm::mat4& transform);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class ComputePipeline {
public:
    struct DataPoint {
        float x;
        float v;
    };

    struct PushConstants {
        float time;
        float dt;
        unsigned data_buffer_size;
        VkDeviceAddress in_buf;
        VkDeviceAddress out_buf;
    };

    void Init(const gfx::CoreCtx& ctx);
    void Compute(VkCommandBuffer cmd,
                 gfx::Device& gfx,
                 const gfx::Buffer& in,
                 const gfx::Buffer& out);
    void Clean(const gfx::CoreCtx& ctx);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

}  // namespace vfs