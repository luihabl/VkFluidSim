#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "pipeline.h"

namespace vfs {
class Renderer {
public:
    void Init(const gfx::Device& gfx, int w, int h);
    void Draw(gfx::Device& gfx, VkCommandBuffer cmd, const gfx::GPUMesh& mesh);
    void Clean(const gfx::Device& gfx);

    const gfx::Image& GetDrawImage() { return draw_img; }

private:
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
    SpriteDrawPipeline sprite_pipeline;
};
}  // namespace vfs