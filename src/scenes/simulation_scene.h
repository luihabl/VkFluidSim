#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "models/model.h"

namespace vfs {
class SimulationScene {
public:
protected:
    glm::vec3 gravity{0.0f, -9.81, 0.0f};

    std::unique_ptr<SPHModel> time_step_model;
    // TODO: add other models here such as the boundary or surface tension
};
}  // namespace vfs
