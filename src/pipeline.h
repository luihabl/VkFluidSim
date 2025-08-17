#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"

namespace vfs {
class SpriteDrawPipeline {
public:
    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clean(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd, gfx::Device& gfx, const gfx::Image& draw_img);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};
}  // namespace vfs