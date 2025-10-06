#pragma once

#include <glm/glm.hpp>

#include "transform.h"

namespace gfx {
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

class OrbitCamera : public Camera {
public:
    void SetAngles(const glm::vec3& angles_deg);
    glm::vec3 GetAngles() const { return camera_angles; }

    void SetRadius(float radius);
    float GetRadius() const { return camera_radius; }

private:
    void Update();
    glm::vec3 camera_angles{0.0f};
    float camera_radius{20.0f};
};

}  // namespace gfx
