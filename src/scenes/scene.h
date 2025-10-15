#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "gfx/camera.h"
#include "gfx/gfx.h"
#include "gfx/transform.h"
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

    virtual void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {}
    virtual void CustomDraw(VkCommandBuffer cmd,
                            const gfx::Image& draw_img,
                            const gfx::Camera& camera) {}

    virtual void DrawDebugUI() {
        if (time_step_model)
            time_step_model->DrawDebugUI();
    }

    SPHModel* GetModel() const { return time_step_model.get(); }

    struct BoxData {
        gfx::Transform transform;
        glm::vec4 color;
        bool hidden{false};
    };

    const std::vector<BoxData>& GetBoxObjs() { return box_draw_objs; }

protected:
    gfx::Device& gfx;
    std::unique_ptr<SPHModel> time_step_model;

    std::vector<BoxData> box_draw_objs;
    // TODO: add other models here such as the boundary or surface tension
};
}  // namespace vfs
