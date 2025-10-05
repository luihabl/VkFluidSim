#pragma once

#include "gfx/mesh.h"
#include "scene.h"

namespace vfs {

class ModelRenderScene final : public SceneBase {
public:
    using SceneBase::SceneBase;

    void Init() override;
    void Step(VkCommandBuffer) override;
    void Clear() override;
    void Reset() override;

private:
    gfx::GPUMesh model_mesh;
    glm::ivec3 fluid_block_size;
};
}  // namespace vfs
