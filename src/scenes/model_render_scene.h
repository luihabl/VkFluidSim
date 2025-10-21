#pragma once

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "pipelines/mesh_pipeline.h"
#include "pipelines/raymarch_pipeline.h"
#include "scene.h"
#include "util/mesh_bvh.h"

namespace vfs {

class ModelRenderScene final : public SceneBase {
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
    void SetQueryPoint(const glm::vec3& point);
    void GenerateSDF();

    // Meshes
    gfx::CPUMesh model_mesh;
    gfx::MeshDrawObj model_draw_obj;

    gfx::CPUMesh box_mesh;
    gfx::MeshDrawObj box_draw_obj;

    // SDF grid
    MeshBVH model_bvh;
    glm::vec3 sdf_min_pos, sdf_max_pos;
    glm::ivec3 sdf_n_cells;
    gfx::Buffer sdf_buffer;
    std::vector<f32> sdf_grid;

    // Draw pipelines
    MeshDrawPipeline mesh_pipeline;
    DensityRaymarchDrawPipeline raymarch_pipeline;

    VkSampler sdf_sampler;
    gfx::Image sdf_image;
};
}  // namespace vfs
