#include "pipeline.h"

#include "gfx/common.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {
void SpriteDrawPipeline::Init(const gfx::CoreCtx& ctx) {
    auto frag = vk::util::LoadShaderModule(
        ctx, Platform::Assets::ResourcePath("shaders/sprite.frag.spv").c_str());
    auto vert = vk::util::LoadShaderModule(
        ctx, Platform::Assets::ResourcePath("shaders/sprite.vert.spv").c_str());
}
}  // namespace vfs