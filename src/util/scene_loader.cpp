#include "scene_loader.h"

#include <fstream>
#include <glm/fwd.hpp>
#include <memory>
#include <nlohmann/json.hpp>

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "models/model.h"
#include "models/wcsph_with_boundary_model.h"
#include "scenes/generic_scene.h"
#include "scenes/scene.h"
#include "simulation.h"

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

void from_json(const json& j, std::vector<GenericScene::FluidBlock>& blocks) {
    blocks.resize(j.size());
    u32 i = 0;
    for (const auto& block : j.items()) {
        block.value()["size"].get_to(blocks[i].size);
        block.value()["pos"].get_to(blocks[i].pos);
        i++;
    }
}

void from_json(const json& j, std::vector<GenericScene::ObjectDef>& objects) {
    objects.resize(j.size());
    u32 i = 0;
    for (const auto& it : j.items()) {
        auto obj = GenericScene::ObjectDef{};

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

    auto fluid_blocks = std::vector<GenericScene::FluidBlock>{};
    data["fluidBlocks"].get_to(fluid_blocks);

    auto boundary_objects = std::vector<GenericScene::ObjectDef>{};
    data["boundaryObjects"].get_to(boundary_objects);

    auto scene = std::make_unique<GenericScene>(gfx, base_parameter, model_parameter, fluid_blocks,
                                                boundary_objects);

    return scene;
}
}  // namespace vfs
