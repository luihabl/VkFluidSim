#include "generic_scene.h"

#include "platform.h"
#include "simulation.h"
#include "util/mesh_loader.h"
#include "util/volume_map.h"

namespace vfs {

GenericScene::GenericScene(gfx::Device& gfx,
                           const SPHModel::Parameters& base_parameters,
                           const WCSPHWithBoundaryModel::Parameters& model_parameters,
                           const std::vector<FluidBlock>& fluid_blocks,
                           const std::vector<ObjectDef>& boundary_objects)
    : SceneBase(gfx) {
    this->base_parameters = base_parameters;
    this->model_parameters = model_parameters;
    this->fluid_blocks = fluid_blocks;
    this->boundary_object_def = boundary_objects;
}
void GenericScene::Init() {
    for (const auto& obj : boundary_object_def) {
        AddBoundaryObject(Platform::Info::ResourcePath(obj.path.c_str()), obj.transform,
                          obj.resolution);
    }
    CreateBoundaryObjectBuffer();

    u32 total_n_particles{0};
    for (const auto& block : fluid_blocks) {
        total_n_particles += glm::compMul(block.size);
    }

    base_parameters.n_particles = total_n_particles;
    model_parameters.n_boundary_objects = boundary_objects.size();
    model_parameters.boundary_objects = boundary_objects_gpu_buffer.device_addr;

    time_step_model = std::make_unique<WCSPHWithBoundaryModel>(&base_parameters, &model_parameters);
    time_step_model->Init(gfx.GetCoreCtx());

    Reset();
}
void GenericScene::Reset() {
    const auto box = time_step_model->GetBoundingBox().value();
    const auto n_particles = time_step_model->GetParameters().n_particles;
    const auto density = time_step_model->GetParameters().target_density;

    auto particle_volume = 1 / density;
    auto particle_diam = std::cbrt(particle_volume);

    u32 offset = 0;
    for (const auto& block : fluid_blocks) {
        u32 count = glm::compMul(block.size);
        glm::vec3 size = particle_diam * static_cast<glm::vec3>(block.size + 1u);
        time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = block.pos}, offset, count);
        offset += count;
    }
}
void GenericScene::Step(VkCommandBuffer cmd) {
    time_step_model->Step(gfx.GetCoreCtx(), cmd);
}
void GenericScene::Clear() {
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
void GenericScene::CustomDraw(VkCommandBuffer cmd,
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
void GenericScene::AddBoundaryObject(const std::string& path,
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
void GenericScene::CreateBoundaryObjectBuffer() {
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
void GenericScene::DrawDebugUI() {
    SceneBase::DrawDebugUI();
}
void GenericScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}
}  // namespace vfs
