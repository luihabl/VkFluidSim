#pragma once

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
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
    glm::uvec3 map_resolution;

    struct VolumeMapBoundaryObject {
        gfx::CPUMesh mesh;
        gfx::GPUMesh gpu_mesh;
        gfx::Transform transform;
        gfx::BoundingBox box;

        MeshSDF sdf;
        LinearLagrangeDiscreteGrid volume_map;
        gfx::Buffer sdf_gpu_grid;
        gfx::Buffer volume_map_gpu_grid;
    };

    // Model render
    // gfx::CPUMesh model_mesh;
    // gfx::MeshDrawObj model_draw_obj;
    MeshDrawPipeline mesh_pipeline;

    gfx::Buffer boundary_objects_gpu_buffer;
    std::vector<VolumeMapBoundaryObject> boundary_objects;

    void AddBoundaryObject(const std::string& path,
                           const gfx::Transform& transform = {},
                           const glm::uvec3 resolution = {20, 20, 20});

    void CreateBoundaryObjectBuffer();

    // Volume map
    // void InitVolumeMap();
    // MeshSDF model_sdf;
    // LinearLagrangeDiscreteGrid volume_map;

    // gfx::Image sdf_img;
    // gfx::Image volume_map_img;
    // VkSampler sdf_linear_sampler_3d;
    // VkSampler vm_linear_sampler_3d;
};

}  // namespace vfs
