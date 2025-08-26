#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "pipeline.h"
#include "platform.h"
#include "renderer.h"

namespace vfs {
class World {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clear();

private:
    gfx::Device gfx;
    Renderer renderer;

    struct FrameBuffers {
        gfx::Buffer position_buffer;
        gfx::Buffer predicted_position_buffer;
        gfx::Buffer velocity_buffer;
        gfx::Buffer density_buffer;
    };

    std::array<FrameBuffers, gfx::FRAME_COUNT> frame_buffers;

    ComputePipeline update_pos_pipeline;
    ComputePipeline boundaries_pipeline;

    gfx::GPUMesh circle_mesh;
    void SetInitialData();

    void SetBox(float w, float h);

    glm::vec4 box;
    glm::vec4 cbox;
    int n_particles = 6000;
    float spacing = 0.2f;
    float scale = 2e-2;
    int current_frame = 0;
};
}  // namespace vfs