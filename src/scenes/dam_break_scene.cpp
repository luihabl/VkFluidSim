#include "dam_break_scene.h"

#include "models/lague_model.h"

namespace vfs {

void DamBreakScene::Init() {
    time_step_model = std::make_unique<LagueModel>();
    time_step_model->Init(gfx.GetCoreCtx(), {
                                                .time_scale = 1.0f,
                                                .fixed_dt = 1.0f / 120.0f,
                                                .iterations = 3,
                                                .n_particles = 400000,
                                                .bounding_box = {.size{23.0f, 10.0f, 10.0f}},
                                            });
    Reset();
}
void DamBreakScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}

void DamBreakScene::Clear() {
    time_step_model->Clear(gfx.GetCoreCtx());
}

void DamBreakScene::Reset() {
    const auto box = time_step_model->GetBoundingBox().value();
    auto size = box.size;
    size.x /= 5.0f;

    auto pos = box.pos;  // + (box.size - size) / 2.0f;
    time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = pos});
}

}  // namespace vfs
