#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "pipeline.h"
#include "platform.h"

namespace vfs {

struct Transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

enum class Projection {
    Orthographic,
    Perspective,
};

class OrthoCamera {
public:
    Transform& GetTransform() { return transform; }

    glm::mat4 ViewProjectionMatrix() {
        auto m = Projection();
        m = glm::translate(m, -transform.position);
        m = glm::scale(m, transform.scale);
        return m;
    }

    glm::mat4 Projection() {
        return glm::ortho(0.0f, (float)Platform::Info::GetConfig()->w,
                          (float)Platform::Info::GetConfig()->h, 0.0f, -1.0f, 1.0f);
    }

private:
    static constexpr glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec2 window_size;
    Transform transform;
};

class SimulationRenderer2D {
public:
    void Init(const gfx::Device& gfx, int w, int h);
    void Draw(gfx::Device& gfx,
              VkCommandBuffer cmd,
              const gfx::GPUMesh& mesh,
              const DrawPushConstants& push_constants,
              uint32_t instances);
    void Clear(const gfx::Device& gfx);

    const gfx::Image& GetDrawImage() const { return draw_img; }

private:
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
    SpriteDrawPipeline sprite_pipeline;
};
}  // namespace vfs
