#pragma once

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/mesh.h"
#include "pipelines/box_pipeline.h"
#include "pipelines/mesh_pipeline.h"
#include "pipelines/raymarch_pipeline.h"
#include "scene.h"
#include "util/discretization.h"
#include "util/mesh_sdf.h"

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
                    const gfx::Camera& camera,
                    const gfx::Transform& global_transform = {}) override;
    void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) override;

private:
    void SetQueryPoint(const glm::vec3& point);

    glm::vec3 query_point;
    SignedDistanceResult signed_distance;
    // MeshBVH::ClosestPointQueryResult query_result;

    // Meshes
    gfx::CPUMesh model_mesh;
    gfx::MeshDrawObj model_draw_obj;

    gfx::CPUMesh box_mesh;
    gfx::MeshDrawObj box_draw_obj;

    // Volume map
    // MeshBVH model_bvh;
    // MeshPseudonormals model_pseudonormals;
    LinearLagrangeDiscreteGrid volume_map;

    MeshSDF model_sdf;
    double sdf_tolerance{0.05};
    glm::ivec3 sdf_n_cells;
    gfx::Buffer sdf_buffer;
    std::vector<f32> sdf_grid;

    // Draw pipelines
    MeshDrawPipeline mesh_pipeline;
    DensityRaymarchDrawPipeline raymarch_pipeline;
    BoxDrawPipeline box_pipeline;

    // gfx::Image sdf_image;
    gfx::DescriptorManager descriptor_manager;
    VkSampler sdf_sampler;
    std::vector<gfx::DescriptorManager::DescriptorInfo> desc_info;
};
}  // namespace vfs
