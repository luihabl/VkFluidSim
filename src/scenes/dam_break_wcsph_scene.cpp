#include "dam_break_wcsph_scene.h"

#include "models/model.h"
#include "models/wcsph_model.h"

namespace vfs {

void DamBreakWCSPHScene::Init() {
    fluid_block_size = {50, 30, 98};

    auto base_parameters = SPHModel::Parameters{
        .time_scale = 1.0f,
        .fixed_dt = 1 / 120.0f,
        .iterations = 3,
        .n_particles = fluid_block_size.x * fluid_block_size.y * fluid_block_size.z,
        .target_density = 1000.0f,
        .bounding_box = {.size{15.0f, 10.0f, 10.0f}},
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
    const auto n_particles = time_step_model->GetParameters().n_particles;
    const auto density = time_step_model->GetParameters().target_density;

    auto particle_volume = 1 / density;
    auto particle_diam = std::cbrt(particle_volume);

    glm::vec3 size = particle_diam * static_cast<glm::vec3>(fluid_block_size + 1);

    auto pos = box.pos;
    pos.z += (box.size.z - size.z) / 2.0f;
    time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = pos});
}

}  // namespace vfs
