#include "camera.h"

namespace gfx {

void Camera::SetPerspective(float fov_x, float z_near, float z_far, float ar) {
    proj_type = ProjType::Perspective;

    fov.x = fov_x;
    aspect_ratio = ar;

    float g = aspect_ratio / glm::tan(fov.x / 2.0);
    fov.y = 2.0f * glm::atan(1.0f / g);

    if (inverse_depth) {
        projection = glm::perspective(fov.y, aspect_ratio, z_far, z_near);
    } else {
        projection = glm::perspective(fov.y, aspect_ratio, z_near, z_far);
    }

    if (clip_space_y_down) {
        projection[1][1] *= -1;
    }
}

// If this is set to true, the .clearValue.depthStencil.depth value in the should be
// set to 0.0f. Also, the compare op should be set to VK_COMPARE_OP_GREATER_OR_EQUAL in the pipeline
// creation
void Camera::SetInverseDepth(bool inverse) {
    inverse_depth = inverse;
    Reset();
}

void Camera::SetFoVX(float fovx) {
    this->fov.x = fovx;
    Reset();
}

void Camera::SetOrtho(float scale, float z_near, float z_far) {
    SetOrtho(glm::vec2(scale), z_near, z_far);
}

void Camera::SetOrtho(const glm::vec2& scale, float z_near, float z_far) {
    proj_type = ProjType::Orthographic;
    ortho_scale = scale;
    this->z_near = z_near;
    this->z_far = z_far;
    aspect_ratio = ortho_scale.x / ortho_scale.y;

    if (inverse_depth) {
        projection =
            glm::ortho(-ortho_scale.x, ortho_scale.x, -ortho_scale.y, ortho_scale.y, z_far, z_near);
    } else {
        projection =
            glm::ortho(-ortho_scale.x, ortho_scale.x, -ortho_scale.y, ortho_scale.y, z_near, z_far);
    }
}

void Camera::SetOrtho2D(const glm::vec2& size, float z_near, float z_far, OriginType origin) {
    proj_type = ProjType::Orthographic2D;
    clip_space_y_down = true;
    ortho_view_size = size;
    this->z_near = z_near;
    this->z_far = z_far;
    aspect_ratio = size.x / size.y;

    switch (origin) {
        case gfx::OriginType::TopLeft: {
            projection = glm::ortho(0.0f, size.x, 0.0f, size.y, z_near, z_far);
            break;
        }
        case gfx::OriginType::BottomLeft: {
            projection = glm::ortho(0.0f, size.x, size.y, 0.0f, z_near, z_far);
            break;
        }
        case gfx::OriginType::Center: {
            auto half = size / 2.0f;
            projection = glm::ortho(-half.x, half.x, half.y, -half.y, z_near, z_far);
            break;
        }
    }
}

void Camera::Reset() {
    switch (proj_type) {
        case gfx::ProjType::Orthographic: {
            SetOrtho(ortho_scale, z_near, z_far);
            break;
        }

        case gfx::ProjType::Orthographic2D: {
            SetOrtho2D(ortho_view_size, z_near, z_far);
            break;
        }

        case gfx::ProjType::Perspective: {
            SetPerspective(fov.x, z_near, z_far, aspect_ratio);
            break;
        }

        default:
            break;
    }
}

void Camera::SetPosition(const glm::vec3& pos) {
    transform.SetPosition(pos);
}

void Camera::SetPosition2D(const glm::vec2& pos) {
    transform.SetPosition(glm::vec3(pos, 0.0f));
}

glm::vec3 Camera::GetPosition() const {
    return transform.Position();
}

void Camera::SetRotation(const glm::quat& q) {
    transform.SetRotation(q);
}

void Camera::SetTarget(const glm::vec3& target) {
    transform.SetRotation(
        glm::quatLookAt(glm::normalize(transform.Position() - target), gfx::axis::UP));
}

const glm::quat& Camera::GetRotation() const {
    return transform.Rotation();
}

glm::mat4 Camera::GetView() const {
    if (proj_type == ProjType::Orthographic2D) {
        return glm::translate(glm::mat4{1.0f}, -transform.Position());
    }

    const auto up = transform.LocalUp();
    const auto pos = GetPosition();
    const auto target = pos + transform.LocalFront();
    return glm::lookAt(pos, target, up);
}

glm::mat4 Camera::GetViewProj() const {
    return projection * GetView();
}

const glm::mat4& Camera::GetProj() const {
    return projection;
}

void OrbitCamera::SetAngles(const glm::vec3& angles) {
    glm::vec3 new_angles = angles;
    new_angles.x = glm::clamp(new_angles.x, -89.f, 89.f);

    if (camera_angles != new_angles) {
        camera_angles = new_angles;
        Update();
    }
}

void OrbitCamera::SetRadius(float radius) {
    if (camera_radius != radius) {
        camera_radius = radius;
        Update();
    }
}

void OrbitCamera::Update() {
    float pitch = glm::radians(camera_angles.x);
    float yaw = glm::radians(camera_angles.y);
    float roll = glm::radians(camera_angles.z);

    auto camera_pos = glm::vec3{
        cos(yaw) * cos(pitch),
        sin(pitch),
        sin(yaw) * cos(pitch),
    };

    SetPosition(camera_radius * camera_pos);
    SetTarget({0.0f, 0.0f, 0.0f});
}
}  // namespace gfx
