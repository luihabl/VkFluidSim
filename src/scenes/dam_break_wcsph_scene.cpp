#include "dam_break_wcsph_scene.h"

#include "models/model.h"
#include "models/wcsph_model.h"

namespace vfs {

void DamBreakWCSPHScene::Init() {
    auto base_parameters = SPHModel::Parameters{
        .time_scale = 1.0f,
        .fixed_dt = 0.001f,
        .iterations = 3,
        .n_particles = 40000,
        .target_density = 200.0f,
        .bounding_box = {.size{23.0f, 10.0f, 10.0f}},
    };

    time_step_model = std::make_unique<WCSPHModel>(&base_parameters);

    time_step_model->Init(gfx.GetCoreCtx());
    Reset();
}
void DamBreakWCSPHScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}

void DamBreakWCSPHScene::Clear() {
    time_step_model->Clear(gfx.GetCoreCtx());
}

void DamBreakWCSPHScene::Reset() {
    const auto box = time_step_model->GetBoundingBox().value();
    auto size = box.size;
    size.x /= 5.0f;

    auto pos = box.pos;  // + (box.size - size) / 2.0f;
    time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = pos});
}

}  // namespace vfs
