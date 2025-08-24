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

    gfx::Buffer *in_buf{nullptr}, *out_buf{nullptr};
    gfx::Buffer b1, b2;
    ComputePipeline comp_pipeline;

    gfx::GPUMesh test_mesh;
    void SetInitialData();

    void SetBox(float w, float h);

    glm::vec4 box;
    glm::vec4 cbox;
    int n_particles = 6000;
    float spacing = 0.2f;
    float scale = 2e-2;
};
}  // namespace vfs