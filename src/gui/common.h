#pragma once

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gfx/common.h"
#include "gfx/mesh.h"

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

    const glm::vec2& GetFoV() const { return fov; }
    void SetFoVX(float fovx);

    void SetRotation(const glm::quat& q);
    void SetTarget(const glm::vec3& target);
    const glm::quat& GetRotation() const;

    bool InverseDepth() const { return inverse_depth; }
    void SetInverseDepth(bool inverse);

    glm::mat4 GetView() const;
    glm::mat4 GetViewProj() const;
    const glm::mat4& GetProj() const;
    Transform& GetTransform() { return transform; }

protected:
    ProjType proj_type{ProjType::None};
    Transform transform;
    glm::mat4 projection;

    // If this is set to true, the .clearValue.depthStencil.depth value in the should be
    // set to 0.0f. Also, the compare op should be set to VK_COMPARE_OP_GREATER_OR_EQUAL in the
    // pipeline creation
    bool inverse_depth{false};
    bool clip_space_y_down{true};

    glm::vec2 fov{glm::radians(90.0f), glm::radians(60.0f)};
    float aspect_ratio{16.0f / 9.0f};
    float z_near{1.0f};
    float z_far{75.0f};

    glm::vec2 ortho_scale{1.0f};
    glm::vec2 ortho_view_size{0.0f};
    void Reset();
};

// class OrbitCamera : public Camera {
// public:
//     glm::vec3 GetAngles() { return camera_angles; }
//     void SetRadius(float radius) { camera_radius = radius; }
//     float GetRadius() const { return camera_radius; }

// private:
//     glm::vec2 initial_mouse_pos{0.0f};
//     glm::vec3 last_camera_angles{0.0f};
//     glm::vec3 camera_angles{0.0f};
//     float camera_radius{0.0f};
// };

void DrawCircleFill(gfx::CPUMesh& mesh, const glm::vec3& center, float radius, int steps);
void DrawQuad(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color);
gfx::Image CreateDrawImage(const gfx::Device& gfx, u32 w, u32 h);
gfx::Image CreateDepthImage(const gfx::Device& gfx, u32 w, u32 h);

}  // namespace vfs
