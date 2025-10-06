#include "transform.h"

namespace gfx {

Transform::Transform(const glm::mat4& matrix) {
    glm::vec3 out_scale;
    glm::quat out_rotation;
    glm::vec3 out_translation;
    glm::vec3 out_skew;
    glm::vec4 out_perspective;
    glm::decompose(matrix, out_scale, out_rotation, out_translation, out_skew, out_perspective);

    position = out_translation;
    rotation = out_rotation;
    scale = out_scale;
    dirty = true;
}

void Transform::SetPosition(const glm::vec3& p) {
    position = p;
    dirty = true;
}

void Transform::SetRotation(const glm::quat& q) {
    rotation = glm::normalize(q);
    dirty = true;
}

void Transform::SetScale(const glm::vec3& s) {
    scale = s;
    dirty = true;
}

const glm::mat4& Transform::Matrix() const {
    if (!dirty) {
        return transform;
    }

    transform = glm::translate(glm::mat4{1.0f}, position);
    if (rotation != glm::identity<glm::quat>()) {
        transform *= glm::mat4_cast(rotation);
    }
    transform = glm::scale(transform, scale);
    dirty = false;
    return transform;
}

bool Transform::IsIdentity() const {
    return Matrix() == glm::mat4{1.0f};
}

Transform Transform::Inverse() const {
    if (IsIdentity()) {
        return Transform{};
    }

    return Transform(glm::inverse(Matrix()));
}

}  // namespace gfx
