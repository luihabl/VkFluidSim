#pragma once

#include "pipelines/mesh_pipeline.h"
#include "scene.h"

namespace vfs {

class DamBreakWithObjectsScene : public SceneBase {
public:
    using SceneBase::SceneBase;

    void Init() override;
    void Step(VkCommandBuffer) override;
    void Clear() override;
    void Reset() override;

    void DrawDebugUI() override;
    void CustomDraw(VkCommandBuffer cmd,
                    const gfx::Image& draw_img,
                    const gfx::Camera& camera) override;
    void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) override;

private:
    glm::ivec3 fluid_block_size;

    // Model render
    gfx::CPUMesh model_mesh;
    gfx::MeshDrawObj model_draw_obj;
    MeshDrawPipeline mesh_pipeline;
};

}  // namespace vfs
