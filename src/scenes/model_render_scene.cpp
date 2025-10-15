#include "model_render_scene.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "imgui.h"
#include "platform.h"
#include "scenes/scene.h"
#include "util/mesh_bvh.h"
#include "util/mesh_loader.h"

namespace vfs {

void ModelRenderScene::Init() {
    auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/suzanne.obj").c_str());
    fmt::println("Number of shapes: {}", obj.size());
    fmt::println("First shape name: {}", obj.front().name);
    fmt::println("First shape number of vertices: {}", obj.front().vertices.size());
    fmt::println("First shape number of indices: {}", obj.front().indices.size());

    mesh = std::move(obj[0]);

    auto model_mesh = gfx::UploadMesh(gfx, mesh);

    auto tr = gfx::Transform();
    // tr.SetRotation(glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)));

    mesh_draw_objs.push_back({
        .mesh = model_mesh,
        .transform = tr,
    });

    bvh.Init(&mesh, MeshBVH::SplitType::BinSplit);
    bvh.Build();

    SetQueryPoint(glm::vec3(1.0f, 1.0f, 1.0f));

    GenerateSDF();
}

void ModelRenderScene::Step(VkCommandBuffer cmd) {}

void ModelRenderScene::DrawDebugUI() {
    SceneBase::DrawDebugUI();

    if (ImGui::CollapsingHeader("Model render scene")) {
        glm::vec3 p = query_point;
        if (ImGui::DragFloat3("Query point", glm::value_ptr(p), 0.01f)) {
            SetQueryPoint(p);
        }
    }
}

void ModelRenderScene::SetQueryPoint(const glm::vec3& point) {
    query_point = point;
    box_draw_objs.resize(2);

    auto point_transform = gfx::Transform{};
    auto scale = glm::vec3(0.02f);
    point_transform.SetScale(scale);
    point_transform.SetPosition(query_point);

    box_draw_objs[0] = {
        .transform = point_transform,
        .color = glm::vec4(1.0, 0.0, 0.0, 1.0),
    };

    auto query_result = bvh.QueryClosestPoint(query_point);

    point_transform.SetPosition(query_result.closest_point);

    box_draw_objs[1] = {
        .transform = point_transform,
        .color = glm::vec4(1.0, 0.0, 0.0, 1.0),
    };
}

void ModelRenderScene::Clear() {
    for (auto& mesh : mesh_draw_objs) {
        mesh.mesh.indices.Destroy();
        mesh.mesh.vertices.Destroy();
    }

    mesh_pipeline.Clear(gfx.GetCoreCtx());
}

namespace {
u32 GetIndex3D(const glm::uvec3& size, const glm::uvec3& idx) {
    return idx.z + size.z * idx.y + size.z * size.y * idx.x;
}
}  // namespace

void ModelRenderScene::GenerateSDF() {
    sdf_n_cells = {10, 10, 10};
    sdf_grid.resize(sdf_n_cells.x * sdf_n_cells.y * sdf_n_cells.z, 0.0f);
    sdf_max_pos = {2, 2, 2};
    sdf_min_pos = {-2, -2, -2};

    auto step = (sdf_max_pos - sdf_min_pos) / ((glm::vec3)sdf_n_cells - 1.0f);

    for (u32 i = 0; i < sdf_n_cells.x; i++) {
        for (u32 j = 0; j < sdf_n_cells.y; j++) {
            for (u32 k = 0; k < sdf_n_cells.z; k++) {
                auto pos = step * glm::vec3(i, j, k) + sdf_min_pos;

                auto res = bvh.QueryClosestPoint(pos);
                sdf_grid[GetIndex3D(sdf_n_cells, {i, j, k})] = std::sqrt(res.min_distance_sq);
            }
        }
    }
}

void ModelRenderScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}

void ModelRenderScene::CustomDraw(VkCommandBuffer cmd,
                                  const gfx::Image& draw_img,
                                  const gfx::Camera& camera) {
    for (const auto& mesh : mesh_draw_objs) {
        mesh_pipeline.Draw(cmd, gfx, draw_img, mesh, camera);
    }
}

void ModelRenderScene::Reset() {}
}  // namespace vfs
