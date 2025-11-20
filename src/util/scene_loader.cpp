#include "scene_loader.h"

#include <fstream>
#include <glm/fwd.hpp>
#include <memory>
#include <nlohmann/json.hpp>

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "mesh_loader.h"
#include "mesh_sdf.h"
#include "models/model.h"
#include "models/wcsph_with_boundary_model.h"
#include "pipelines/mesh_pipeline.h"
#include "platform.h"
#include "scenes/scene.h"
#include "simulation.h"
#include "volume_map.h"

using json = nlohmann::json;

namespace glm {
void from_json(const json& j, vec3& vec) {
    j.at(0).get_to(vec.x);
    j.at(1).get_to(vec.y);
    j.at(2).get_to(vec.z);
}

void from_json(const json& j, uvec3& vec) {
    j.at(0).get_to(vec.x);
    j.at(1).get_to(vec.y);
    j.at(2).get_to(vec.z);
}
}  // namespace glm

namespace gfx {
void from_json(const json& j, gfx::BoundingBox& bb) {
    j.at("pos").get_to(bb.pos);
    j.at("size").get_to(bb.size);
}
}  // namespace gfx

namespace vfs {

void from_json(const json& j, SimulationParameters& par) {
    if (j.contains("gravity"))
        j.at("gravity").get_to(par.gravity);

    if (j.contains("smoothRadius"))
        j.at("smoothRadius").get_to(par.smooth_radius);
}

void from_json(const json& j, SPHModel::Parameters& par) {
    j.at("timeScale").get_to(par.time_scale);
    j.at("iterations").get_to(par.iterations);
    j.at("dt").get_to(par.fixed_dt);
    j.at("targetDensity").get_to(par.target_density);
    j.at("boundingBox").get_to(par.bounding_box);
}

void from_json(const json& j, WCSPHWithBoundaryModel::Parameters& par) {
    j.at("stiffness").get_to(par.stiffness);
    j.at("expoent").get_to(par.expoent);
    j.at("viscosityStrenght").get_to(par.viscosity_strenght);
    j.at("outerWallStiffness").get_to(par.wall_stiffness);
}

struct FluidBlock {
    glm::uvec3 size;
    glm::vec3 pos;
};

void from_json(const json& j, std::vector<FluidBlock>& blocks) {
    blocks.resize(j.size());
    u32 i = 0;
    for (const auto& block : j.items()) {
        block.value()["size"].get_to(blocks[i].size);
        block.value()["pos"].get_to(blocks[i].pos);
        i++;
    }
}

struct ObjectDef {
    std::string path;
    gfx::Transform transform;
    glm::uvec3 resolution;
};

void from_json(const json& j, std::vector<ObjectDef>& objects) {
    objects.resize(j.size());
    u32 i = 0;
    for (const auto& it : j.items()) {
        auto obj = ObjectDef{};

        auto& o = it.value();

        o.at("resourcePath").get_to(obj.path);

        obj.transform = gfx::Transform{};

        if (o.contains("position")) {
            glm::vec3 pos;
            o.at("position").get_to(pos);
            obj.transform.SetPosition(pos);
        }

        if (o.contains("rotation")) {
            float angle;
            glm::vec3 axis;
            o.at("rotation").at("angle").get_to(angle);
            o.at("rotation").at("axis").get_to(axis);
            obj.transform.SetRotation(glm::rotate(glm::radians(angle), axis));
        }

        if (o.contains("scale")) {
            glm::vec3 scale;
            o.at("scale").get_to(scale);
            obj.transform.SetScale(scale);
        }

        o.at("volumeMapResolution").get_to(obj.resolution);

        objects[i++] = obj;
    }
}

class GenericScene : public SceneBase {
public:
    using SceneBase::SceneBase;

    GenericScene(gfx::Device& gfx,
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

    void Init() override {
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

        time_step_model =
            std::make_unique<WCSPHWithBoundaryModel>(&base_parameters, &model_parameters);
        time_step_model->Init(gfx.GetCoreCtx());

        Reset();
    }

    void Reset() override {
        const auto box = time_step_model->GetBoundingBox().value();
        const auto n_particles = time_step_model->GetParameters().n_particles;
        const auto density = time_step_model->GetParameters().target_density;

        auto particle_volume = 1 / density;
        auto particle_diam = std::cbrt(particle_volume);

        u32 offset = 0;
        for (const auto& block : fluid_blocks) {
            u32 count = glm::compMul(block.size);
            glm::vec3 size = particle_diam * static_cast<glm::vec3>(block.size + 1u);
            time_step_model->SetParticlesInBox(gfx, {.size = size, .pos = block.pos}, offset,
                                               count);
            offset += count;
        }
    }

    void Step(VkCommandBuffer cmd) override { time_step_model->Step(gfx.GetCoreCtx(), cmd); }

    void Clear() override {
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

    void CustomDraw(VkCommandBuffer cmd,
                    const gfx::Image& draw_img,
                    const gfx::Camera& camera,
                    const gfx::Transform& global_transform) override {
        for (const auto& b : boundary_objects) {
            auto o = gfx::MeshDrawObj{
                .mesh = b.gpu_mesh,
                .transform = global_transform.Matrix() * b.transform.Matrix(),
            };

            mesh_pipeline.Draw(cmd, gfx, draw_img, o, camera);
        }
    }

    void InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) override {
        mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
    }

    void DrawDebugUI() override { SceneBase::DrawDebugUI(); }

private:
    struct VolumeMapBoundaryObject {
        gfx::CPUMesh mesh;
        gfx::GPUMesh gpu_mesh;
        gfx::Transform transform;
        gfx::BoundingBox box;

        MeshSDF sdf;
        LinearLagrangeDiscreteGrid volume_map;
        gfx::Buffer sdf_gpu_grid;
        gfx::Buffer volume_map_gpu_grid;
    };

    std::string name;
    SPHModel::Parameters base_parameters;
    WCSPHWithBoundaryModel::Parameters model_parameters;

    gfx::Buffer boundary_objects_gpu_buffer;
    std::vector<VolumeMapBoundaryObject> boundary_objects;

    std::vector<ObjectDef> boundary_object_def;
    std::vector<FluidBlock> fluid_blocks;

    MeshDrawPipeline mesh_pipeline;

    void AddBoundaryObject(const std::string& path,
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

    void CreateBoundaryObjectBuffer() {
        boundary_objects_gpu_buffer =
            gfx::CreateDataBuffer<WCSPHWithBoundaryModel::BoundaryObjectInfo>(
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
};

std::unique_ptr<SceneBase> LoadScene(gfx::Device& gfx, const std::string& config_path) {
    using json = nlohmann::json;

    std::ifstream f(config_path);
    json data = json::parse(f);

    auto& sim = Simulation::Get();
    auto global_parameters = sim.GetGlobalParameters();
    data["simulationParameters"].get_to(global_parameters);
    Simulation::Get().SetGlobalParameters(global_parameters);

    SPHModel::Parameters base_parameter{};
    data["simulationParameters"].get_to(base_parameter);

    WCSPHWithBoundaryModel::Parameters model_parameter{};
    data["wcsphParameters"].get_to(model_parameter);

    auto fluid_blocks = std::vector<FluidBlock>{};
    data["fluidBlocks"].get_to(fluid_blocks);

    auto boundary_objects = std::vector<ObjectDef>{};
    data["boundaryObjects"].get_to(boundary_objects);

    auto scene = std::make_unique<GenericScene>(gfx, base_parameter, model_parameter, fluid_blocks,
                                                boundary_objects);

    return scene;
}
}  // namespace vfs
