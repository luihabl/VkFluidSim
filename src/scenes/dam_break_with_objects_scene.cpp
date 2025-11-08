#include "dam_break_with_objects_scene.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>

#include "gfx/descriptor.h"
#include "gfx/transform.h"
#include "models/model.h"
#include "models/wcsph_with_boundary_model.h"
#include "platform.h"
#include "simulation.h"
#include "util/mesh_loader.h"
#include "util/volume_map.h"

namespace vfs {

void DamBreakWithObjectsScene::Init() {
    auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/suzanne.obj").c_str());
    model_mesh = std::move(obj[0]);

    auto monkey_transform = gfx::Transform{};
    monkey_transform.SetPosition({10, 1, 5});
    monkey_transform.SetRotation(glm::rotate(glm::radians(-90.0f), glm::vec3{0, 1, 0}));

    model_draw_obj = {
        .mesh = gfx::UploadMesh(gfx, model_mesh),
        .transform = monkey_transform,
    };

    auto info = InitVolumeMap();

    fluid_block_size = {50, 30, 98};

    auto base_parameters = SPHModel::Parameters{
        .time_scale = 1.0f,
        .fixed_dt = 1 / 120.0f,
        .iterations = 3,
        .n_particles = fluid_block_size.x * fluid_block_size.y * fluid_block_size.z,
        .target_density = 1000.0f,
        .bounding_box = {.size{15.0f, 10.0f, 10.0f}},
    };

    auto model_parameters = WCSPHWithBoundaryModel::Parameters{
        .wall_stiffness = 1e4,
        .stiffness = 1000,
        .expoent = 7,
        .viscosity_strenght = 0.01,
        .boundary_object =
            {
                .transform = glm::inverse(monkey_transform.Matrix()),
                .box = model_sdf.GetBox(),
            },
    };

    time_step_model =
        std::make_unique<WCSPHWithBoundaryModel>(info, &base_parameters, &model_parameters);
    time_step_model->Init(gfx.GetCoreCtx());
    Reset();
}

std::vector<gfx::DescriptorManager::DescriptorInfo> DamBreakWithObjectsScene::InitVolumeMap() {
    auto& sim = Simulation::Get();

    map_resolution = {20, 20, 20};

    model_sdf.Init(model_mesh, map_resolution, {}, 0.05);
    model_sdf.Build();

    GenerateVolumeMap(model_sdf, sim.GetGlobalParameters().smooth_radius, volume_map);

    // Generate SDF image
    sdf_img = gfx::Image::Create(gfx.GetCoreCtx(),
                                 {
                                     .width = map_resolution.x,
                                     .height = map_resolution.y,
                                     .depth = map_resolution.z,
                                 },
                                 VK_FORMAT_R32_SFLOAT,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    {
        auto sdf_f32_grid = std::vector<f32>(model_sdf.GetSDF().size());
        for (u32 i = 0; i < sdf_f32_grid.size(); i++) {
            sdf_f32_grid[i] = (f32)model_sdf.GetSDF()[i];
        }
        gfx.SetImageData(sdf_img, sdf_f32_grid.data(), sizeof(float));
    }

    // Generate volume map image
    volume_map_img = gfx::Image::Create(
        gfx.GetCoreCtx(),
        {
            .width = map_resolution.x,
            .height = map_resolution.y,
            .depth = map_resolution.z,
        },
        VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    {
        auto vm_f32_grid = std::vector<f32>(model_sdf.GetSDF().size());
        for (u32 i = 0; i < vm_f32_grid.size(); i++) {
            vm_f32_grid[i] = (f32)volume_map.GetGrid()[i];
        }
        gfx.SetImageData(volume_map_img, vm_f32_grid.data(), sizeof(float));
    }

    // Sampler
    auto sampler_create_info = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
    };

    vkCreateSampler(gfx.GetCoreCtx().device, &sampler_create_info, nullptr, &linear_sampler_3d);

    std::vector<gfx::DescriptorManager::DescriptorInfo> info;

    info.push_back({
        .type = gfx::DescriptorManager::DescType::CombinedImageSampler,
        .image =
            {
                .image = sdf_img,
                .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .sampler = linear_sampler_3d,
            },
    });

    info.push_back({
        .type = gfx::DescriptorManager::DescType::CombinedImageSampler,
        .image =
            {
                .image = volume_map_img,
                .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .sampler = linear_sampler_3d,
            },
    });

    return info;
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
                                          const gfx::Camera& camera,
                                          const gfx::Transform& global_transform) {
    auto obj = model_draw_obj;
    obj.transform = global_transform.Matrix() * model_draw_obj.transform.Matrix();
    mesh_pipeline.Draw(cmd, gfx, draw_img, obj, camera);
}

void DamBreakWithObjectsScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}

}  // namespace vfs
