#pragma once

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common.h"
#include "draw_pipelines.h"
#include "gfx/common.h"
#include "gfx/gfx.h"
#include "models/lague_model_2d.h"

namespace vfs {

class SimulationRenderer2D {
public:
    void Init(const gfx::Device& gfx, const Simulation2D& simulation, int w, int h);
    void Draw(gfx::Device& gfx,
              VkCommandBuffer cmd,
              const Simulation2D& simulation,
              u32 current_frame,
              const glm::mat4 view_proj);
    void Clear(const gfx::Device& gfx);
    Transform& GetTransform() { return transform; }

    const gfx::Image& GetDrawImage() const { return draw_img; }

private:
    Transform box_transform;
    Transform transform;
    gfx::GPUMesh particle_mesh;
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
    ParticleDrawPipeline sprite_pipeline;
    BoxDrawPipeline box_pipeline;
};

}  // namespace vfs
