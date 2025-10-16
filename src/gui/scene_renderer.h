#pragma once

#include "gfx/camera.h"
#include "gfx/gfx.h"
#include "gfx/transform.h"
#include "models/model.h"
#include "pipelines/box_pipeline.h"
#include "pipelines/particle_pipeline.h"
#include "scenes/scene.h"

namespace vfs {

class SceneRenderer {
public:
    void Init(const gfx::Device& gfx, SceneBase* scene, int w, int h);
    void Draw(gfx::Device& gfx, VkCommandBuffer cmd, const gfx::Camera& camera);
    void Clear(const gfx::Device& gfx);
    gfx::Transform& GetTransform() { return transform; }
    const gfx::Image& GetDrawImage() const { return draw_img; }

private:
    SceneBase* scene{nullptr};

    SPHModel::DataBuffers render_buffers;

    gfx::Transform box_transform;
    gfx::Transform sim_transform;
    gfx::Transform transform;
    gfx::GPUMesh particle_mesh;
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
    Particle3DDrawPipeline particles_pipeline;
    BoxDrawPipeline box_pipeline;
};
}  // namespace vfs
