#pragma once

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gfx/common.h"

namespace gfx {

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

}  // namespace gfx
