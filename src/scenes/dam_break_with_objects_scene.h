#pragma once

#include "gfx/descriptor.h"
#include "pipelines/mesh_pipeline.h"
#include "scene.h"
#include "util/mesh_sdf.h"

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
                    const gfx::Camera& camera,
                    const gfx::Transform& global_transform = {}) override;
    void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) override;

private:
    glm::ivec3 fluid_block_size;

    // Model render
    gfx::CPUMesh model_mesh;
    gfx::MeshDrawObj model_draw_obj;
    MeshDrawPipeline mesh_pipeline;

    // Volume map

    std::vector<gfx::DescriptorManager::DescriptorInfo> InitVolumeMap();
    glm::uvec3 map_resolution;
    MeshSDF model_sdf;
    LinearLagrangeDiscreteGrid volume_map;

    gfx::Image sdf_img;
    gfx::Image volume_map_img;
    VkSampler linear_sampler_3d;
};

}  // namespace vfs
