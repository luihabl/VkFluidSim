#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "gfx/gfx.h"
#include "gfx/mesh.h"
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
    const std::vector<gfx::MeshDrawObj>& GetMeshDrawObjs() { return mesh_draw_objs; }

protected:
    gfx::Device& gfx;
    std::unique_ptr<SPHModel> time_step_model;
    std::vector<gfx::MeshDrawObj> mesh_draw_objs;
    // TODO: add other models here such as the boundary or surface tension
};
}  // namespace vfs
