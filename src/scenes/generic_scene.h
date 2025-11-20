#pragma once

#include <glm/glm.hpp>
#include <string>

#include "gfx/transform.h"
#include "models/wcsph_with_boundary_model.h"
#include "pipelines/mesh_pipeline.h"
#include "scenes/scene.h"
#include "util/mesh_sdf.h"

namespace vfs {

class GenericScene : public SceneBase {
public:
    using SceneBase::SceneBase;

    struct FluidBlock {
        glm::uvec3 size;
        glm::vec3 pos;
    };

    struct ObjectDef {
        std::string path;
        gfx::Transform transform;
        glm::uvec3 resolution;
    };

    GenericScene(gfx::Device& gfx,
                 const SPHModel::Parameters& base_parameters,
                 const WCSPHWithBoundaryModel::Parameters& model_parameters,
                 const std::vector<FluidBlock>& fluid_blocks,
                 const std::vector<ObjectDef>& boundary_objects);

    void Init() override;

    void Reset() override;

    void Step(VkCommandBuffer cmd) override;

    void Clear() override;

    void CustomDraw(VkCommandBuffer cmd,
                    const gfx::Image& draw_img,
                    const gfx::Camera& camera,
                    const gfx::Transform& global_transform) override;

    void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) override;

    void DrawDebugUI() override;

private:
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

    std::string name;
    SPHModel::Parameters base_parameters;
    WCSPHWithBoundaryModel::Parameters model_parameters;

    gfx::Buffer boundary_objects_gpu_buffer;
    std::vector<VolumeMapBoundaryObject> boundary_objects;

    std::vector<ObjectDef> boundary_object_def;
    std::vector<FluidBlock> fluid_blocks;

    MeshDrawPipeline mesh_pipeline;

    void AddBoundaryObject(const std::string& path,
                           const gfx::Transform& transform,
                           const glm::uvec3 resolution);

    void CreateBoundaryObjectBuffer();
};
}  // namespace vfs
