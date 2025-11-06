#include "dam_break_with_objects_scene.h"

#include <glm/ext/matrix_transform.hpp>

#include "gfx/transform.h"
#include "models/model.h"
#include "models/wcsph_model.h"
#include "platform.h"
#include "util/mesh_loader.h"

namespace vfs {

void DamBreakWithObjectsScene::Init() {
    {
        auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/suzanne.obj").c_str());
        model_mesh = std::move(obj[0]);

        auto monkey_transform = gfx::Transform{};
        monkey_transform.SetPosition({3, -4, 0});
        monkey_transform.SetRotation(glm::rotate(glm::radians(-90.0f), glm::vec3{0, 1, 0}));

        model_draw_obj = {
            .mesh = gfx::UploadMesh(gfx, model_mesh),
            .transform = monkey_transform,
        };
    }

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
void DamBreakWithObjectsScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}

void DamBreakWithObjectsScene::Clear() {
    time_step_model->Clear(gfx.GetCoreCtx());
    model_draw_obj.mesh.indices.Destroy();
    model_draw_obj.mesh.vertices.Destroy();
    mesh_pipeline.Clear(gfx.GetCoreCtx());
}

void DamBreakWithObjectsScene::Reset() {
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

void DamBreakWithObjectsScene::DrawDebugUI() {
    SceneBase::DrawDebugUI();
}

void DamBreakWithObjectsScene::CustomDraw(VkCommandBuffer cmd,
                                          const gfx::Image& draw_img,
                                          const gfx::Camera& camera) {
    mesh_pipeline.Draw(cmd, gfx, draw_img, model_draw_obj, camera);
}

void DamBreakWithObjectsScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}

}  // namespace vfs
