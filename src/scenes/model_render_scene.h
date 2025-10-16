#pragma once

#include "gfx/mesh.h"
#include "pipelines/mesh_pipeline.h"
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
    gfx::CPUMesh mesh;
    MeshBVH bvh;
    glm::vec3 query_point;
    void SetQueryPoint(const glm::vec3& point);

    void GenerateSDF();

    // SDF grid
    glm::vec3 sdf_min_pos, sdf_max_pos;
    glm::uvec3 sdf_n_cells;
    std::vector<f32> sdf_grid;

    // Draw pipelines
    std::vector<gfx::MeshDrawObj> mesh_draw_objs;
    MeshDrawPipeline mesh_pipeline;
};
}  // namespace vfs
