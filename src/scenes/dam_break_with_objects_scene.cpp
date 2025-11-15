#include "dam_break_with_objects_scene.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "models/model.h"
#include "models/wcsph_with_boundary_model.h"
#include "platform.h"
#include "simulation.h"
#include "util/mesh_loader.h"
#include "util/volume_map.h"

namespace vfs {

void DamBreakWithObjectsScene::AddBoundaryObject(const std::string& path,
                                                 const gfx::Transform& transform,
                                                 const glm::uvec3 resolution) {
    VolumeMapBoundaryObject obj;

    auto meshes = LoadObjMesh(path.c_str());
    obj.mesh = std::move(meshes.back());
    obj.transform = transform;
    obj.gpu_mesh = gfx::UploadMesh(gfx, obj.mesh);

    auto& sim = Simulation::Get();

    obj.sdf.Init(obj.mesh, resolution, 0.0, 8.0 * sim.GetGlobalParameters().smooth_radius);
    obj.sdf.Build();

    GenerateVolumeMap(obj.sdf, sim.GetGlobalParameters().smooth_radius, obj.volume_map);

    obj.sdf_gpu_grid = gfx::CreateDataBuffer<float>(gfx.GetCoreCtx(), obj.sdf.GetSDF().size());
    obj.volume_map_gpu_grid =
        gfx::CreateDataBuffer<float>(gfx.GetCoreCtx(), obj.volume_map.GetGrid().size());

    auto tmp = std::vector<f32>(obj.volume_map.GetGrid().size());

    for (u32 i = 0; i < tmp.size(); i++) {
        tmp[i] = (f32)obj.sdf.GetSDF()[i];
    }

    gfx.SetDataVec(obj.sdf_gpu_grid, tmp);

    for (u32 i = 0; i < tmp.size(); i++) {
        tmp[i] = (f32)obj.volume_map.GetGrid()[i];
    }

    gfx.SetDataVec(obj.volume_map_gpu_grid, tmp);

    boundary_objects.push_back(obj);
}

void DamBreakWithObjectsScene::CreateBoundaryObjectBuffer() {
    boundary_objects_gpu_buffer = gfx::CreateDataBuffer<WCSPHWithBoundaryModel::BoundaryObjectInfo>(
        gfx.GetCoreCtx(), boundary_objects.size());

    auto objs = std::vector<WCSPHWithBoundaryModel::BoundaryObjectInfo>();
    for (const auto& b : boundary_objects) {
        objs.push_back({
            .transform = glm::inverse(b.transform.Matrix()),
            .rotation = glm::mat4_cast(b.transform.Rotation()),
            .box = b.sdf.GetBox(),
            .sdf_grid = b.sdf_gpu_grid.device_addr,
            .volume_map_grid = b.volume_map_gpu_grid.device_addr,
            .resolution = b.sdf.GetResolution(),
        });
    }

    gfx.SetDataVec(boundary_objects_gpu_buffer, objs);
}

void DamBreakWithObjectsScene::Init() {
    auto transform = gfx::Transform{};
    transform.SetPosition({5.5, 2.9, 6.1});
    transform.SetRotation(glm::rotate(glm::radians(-90.0f), glm::vec3{0, 1, 0}));
    transform.SetScale({4, 3, 0.5});

    AddBoundaryObject(Platform::Info::ResourcePath("models/box.obj"), transform, glm::uvec3(30));
    CreateBoundaryObjectBuffer();

    // auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/box.obj").c_str());
    // model_mesh = std::move(obj[0]);

    // auto monkey_transform = gfx::Transform{};
    // monkey_transform.SetPosition({5.5, 2.9, 6.1});
    // monkey_transform.SetRotation(glm::rotate(glm::radians(-90.0f), glm::vec3{0, 1, 0}));
    // monkey_transform.SetScale({4, 3, 0.5});

    // model_draw_obj = {
    //     .mesh = gfx::UploadMesh(gfx, model_mesh),
    //     .transform = monkey_transform,
    // };

    // auto info = InitVolumeMap();

    fluid_block_size = {50, 20, 98};

    auto base_parameters = SPHModel::Parameters{
        .time_scale = 1.0f,
        .fixed_dt = 1 / 120.0f,
        .iterations = 3,
        .n_particles = fluid_block_size.x * fluid_block_size.y * fluid_block_size.z,
        .target_density = 1000.0f,
        .bounding_box = {.size{10.0f, 10.0f, 10.0f}},
    };

    auto model_parameters = WCSPHWithBoundaryModel::Parameters{
        .wall_stiffness = 1e4,
        .stiffness = 1000,
        .expoent = 7,
        .viscosity_strenght = 0.01,
        .boundary_objects = boundary_objects_gpu_buffer.device_addr,
        .n_boundary_objects = (u32)boundary_objects.size(),
    };

    time_step_model = std::make_unique<WCSPHWithBoundaryModel>(&base_parameters, &model_parameters);
    time_step_model->Init(gfx.GetCoreCtx());
    Reset();
}

// std::vector<gfx::DescriptorManager::DescriptorInfo> DamBreakWithObjectsScene::InitVolumeMap() {
//     auto& sim = Simulation::Get();

//     map_resolution = glm::uvec3(50);

//     model_sdf.Init(model_mesh, map_resolution, 0.0,
//                    8.0 * Simulation::Get().GetGlobalParameters().smooth_radius);
//     model_sdf.Build();

//     GenerateVolumeMap(model_sdf, sim.GetGlobalParameters().smooth_radius, volume_map);

//     // Generate SDF image
//     sdf_img = gfx::Image::Create(gfx.GetCoreCtx(),
//                                  {
//                                      .width = map_resolution.x,
//                                      .height = map_resolution.y,
//                                      .depth = map_resolution.z,
//                                  },
//                                  VK_FORMAT_R32_SFLOAT,
//                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

//     {
//         auto sdf_f32_grid = std::vector<f32>(model_sdf.GetSDF().size());
//         for (u32 i = 0; i < sdf_f32_grid.size(); i++) {
//             sdf_f32_grid[i] = (f32)model_sdf.GetSDF()[i];
//         }
//         gfx.SetImageData(sdf_img, sdf_f32_grid.data(), sizeof(float));
//     }

//     // Generate volume map image
//     volume_map_img = gfx::Image::Create(
//         gfx.GetCoreCtx(),
//         {
//             .width = map_resolution.x,
//             .height = map_resolution.y,
//             .depth = map_resolution.z,
//         },
//         VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

//     {
//         auto vm_f32_grid = std::vector<f32>(model_sdf.GetSDF().size());
//         for (u32 i = 0; i < vm_f32_grid.size(); i++) {
//             vm_f32_grid[i] = (f32)volume_map.GetGrid()[i];
//         }
//         gfx.SetImageData(volume_map_img, vm_f32_grid.data(), sizeof(float));
//     }

//     // Sampler
//     auto sampler_create_info = VkSamplerCreateInfo{
//         .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
//         .minFilter = VK_FILTER_LINEAR,
//         .magFilter = VK_FILTER_LINEAR,
//     };

//     vkCreateSampler(gfx.GetCoreCtx().device, &sampler_create_info, nullptr,
//     &sdf_linear_sampler_3d); vkCreateSampler(gfx.GetCoreCtx().device, &sampler_create_info,
//     nullptr, &vm_linear_sampler_3d);

//     std::vector<gfx::DescriptorManager::DescriptorInfo> info;

//     info.push_back({
//         .type = gfx::DescriptorManager::DescType::CombinedImageSampler,
//         .image =
//             {
//                 .image = sdf_img,
//                 .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                 .sampler = sdf_linear_sampler_3d,
//             },
//     });

//     info.push_back({
//         .type = gfx::DescriptorManager::DescType::CombinedImageSampler,
//         .image =
//             {
//                 .image = volume_map_img,
//                 .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                 .sampler = vm_linear_sampler_3d,
//             },
//     });

//     return info;
// }

void DamBreakWithObjectsScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}

void DamBreakWithObjectsScene::Clear() {
    time_step_model->Clear(gfx.GetCoreCtx());

    boundary_objects_gpu_buffer.Destroy();
    for (auto& b : boundary_objects) {
        b.gpu_mesh.vertices.Destroy();
        b.gpu_mesh.indices.Destroy();
        b.sdf.Clean();
        b.sdf_gpu_grid.Destroy();
        b.volume_map_gpu_grid.Destroy();
    }

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
    for (const auto& b : boundary_objects) {
        auto o = gfx::MeshDrawObj{
            .mesh = b.gpu_mesh,
            .transform = global_transform.Matrix() * b.transform.Matrix(),
        };

        mesh_pipeline.Draw(cmd, gfx, draw_img, o, camera);
    }
}

void DamBreakWithObjectsScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}

}  // namespace vfs
