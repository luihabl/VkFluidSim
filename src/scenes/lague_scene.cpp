#include "lague_scene.h"

#include "models/lague_model.h"

namespace vfs {

void LagueSimulationScene::Init() {
    time_step_model = std::make_unique<LagueModel>();
    time_step_model->Init(gfx.GetCoreCtx());
    Reset();
}
void LagueSimulationScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}

void LagueSimulationScene::Clear() {
    time_step_model->Clear(gfx.GetCoreCtx());
}

void LagueSimulationScene::Reset() {
    const auto box = time_step_model->GetBoundingBox().value();
    auto size = box.size;
    size.x /= 5.0f;

    auto pos = box.pos;  // + (box.size - size) / 2.0f;
    time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = pos});
}

}  // namespace vfs
