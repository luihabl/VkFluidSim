#pragma once

#include "scene.h"

namespace vfs {

class DamBreakWCSPHScene : public SceneBase {
public:
    using SceneBase::SceneBase;

    void Init() override;
    void Step(VkCommandBuffer) override;
    void Clear() override;
    void Reset() override;

private:
    glm::ivec3 fluid_block_size;
};

}  // namespace vfs
