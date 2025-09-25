#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "gfx/gfx.h"
#include "models/model.h"

namespace vfs {
class SceneBase {
public:
    SceneBase(gfx::Device& device) : gfx(device) {};
    virtual ~SceneBase() {}
    virtual void Init() = 0;
    virtual void Step(VkCommandBuffer) = 0;
    virtual void Clear() = 0;
    virtual void Reset() = 0;

    SPHModel* GetModel() const { return time_step_model.get(); }

protected:
    gfx::Device& gfx;
    std::unique_ptr<SPHModel> time_step_model;
    // TODO: add other models here such as the boundary or surface tension
};
}  // namespace vfs
