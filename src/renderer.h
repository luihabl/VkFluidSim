#pragma once

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "pipeline.h"
#include "simulation.h"

namespace vfs {

class Transform {
public:
    Transform() = default;
    Transform(const glm::mat4& matrix);

    void SetPosition(const glm::vec3& p);
    const auto& Position() const { return position; }

    void SetRotation(const glm::quat& q);
    const auto& Rotation() const { return rotation; };

    void SetScale(const glm::vec3& s);
    const auto& Scale() const { return scale; };

    auto LocalUp() const { return rotation * gfx::axis::UP; }
    auto LocalFront() const { return rotation * gfx::axis::FRONT; }
    auto LocalRight() const { return rotation * gfx::axis::RIGHT; }

    const glm::mat4& Matrix() const;
    Transform Inverse() const;
    bool IsIdentity() const;

private:
    mutable bool dirty{true};
    mutable glm::mat4 transform;

    glm::vec3 position{0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    glm::quat rotation{glm::identity<glm::quat>()};
};

enum class ProjType {
    None,
    Orthographic,
    Orthographic2D,
    Perspective,
};

enum class OriginType {
    Center,
    TopLeft,
    BottomLeft,
};

class Camera {
public:
    void SetPerspective(float fov_x, float z_near, float z_far, float aspect_ratio);
    void SetOrtho(float scale, float z_near, float z_far);
    void SetOrtho(const glm::vec2& scale, float z_near, float z_far);
    void SetOrtho2D(const glm::vec2& size,
                    float z_near,
                    float z_far,
                    OriginType origin = OriginType::TopLeft);

    void SetPosition(const glm::vec3& pos);
    void SetPosition2D(const glm::vec2& pos);
    glm::vec3 GetPosition() const;

    void SetRotation(const glm::quat& q);
    const glm::quat& GetRotation() const;

    void SetTarget(const glm::vec3& target);

    glm::mat4 GetView() const;
    glm::mat4 GetViewProj() const;
    const glm::mat4& GetProj() const;

private:
    ProjType proj_type{ProjType::None};
    Transform transform;
    glm::mat4 projection;

    bool inverse_depth{false};
    bool clip_space_y_down{true};

    float fov_x{glm::radians(90.0f)};
    float fov_y{glm::radians(60.0f)};
    float aspect_ratio{16.0f / 9.0f};
    float z_near{1.0f};
    float z_far{75.0f};

    glm::vec2 ortho_scale{1.0f};
    glm::vec2 ortho_view_size{0.0f};
};

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

class SimulationRenderer3D {
public:
    void Init(const gfx::Device& gfx, int w, int h);
    void Draw(gfx::Device& gfx, VkCommandBuffer cmd, const glm::mat4 view_proj);
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
    // Particle3DDrawPipeline sprite_pipeline;
    BoxDrawPipeline box_pipeline;
};

}  // namespace vfs
