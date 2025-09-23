#pragma once

#include "common.h"
#include "gfx/gfx.h"
#include "models/model.h"

namespace vfs {

class SimulationRenderer3D {
public:
    void Init(const gfx::Device& gfx, const SPHModel* simulation, int w, int h);
    void Draw(gfx::Device& gfx,
              VkCommandBuffer cmd,
              const SPHModel* simulation,
              const Camera& camera);
    void Clear(const gfx::Device& gfx);
    Transform& GetTransform() { return transform; }
    const gfx::Image& GetDrawImage() const { return draw_img; }

private:
    SPHModel::DataBuffers render_buffers;

    Transform box_transform;
    Transform sim_transform;
    Transform transform;
    gfx::GPUMesh particle_mesh;
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
    Particle3DDrawPipeline particles_pipeline;
    BoxDrawPipeline box_pipeline;
};
}  // namespace vfs
