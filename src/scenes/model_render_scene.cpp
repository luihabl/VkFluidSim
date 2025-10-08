#include "model_render_scene.h"

#include <glm/ext/matrix_transform.hpp>

#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "platform.h"
#include "util/mesh_bvh.h"
#include "util/mesh_loader.h"

namespace vfs {
void ModelRenderScene::Init() {
    auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/suzanne.obj").c_str());
    fmt::println("Number of shapes: {}", obj.size());
    fmt::println("First shape name: {}", obj.front().name);
    fmt::println("First shape number of vertices: {}", obj.front().vertices.size());
    fmt::println("First shape number of indices: {}", obj.front().indices.size());

    auto model_mesh = gfx::UploadMesh(gfx, obj.front());

    auto tr = gfx::Transform();
    tr.SetRotation(glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)));

    mesh_draw_objs.push_back({
        .mesh = model_mesh,
        .transform = tr,
    });

    MeshBVH bvh;
    bvh.Init(&obj[0]);
    bvh.Build();
    auto& bvh_nodes = bvh.GetNodes();

    auto bbox = bvh_nodes.front().bbox;
    auto box_transform = gfx::Transform{};
    box_transform.SetRotation(glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)));
    box_transform.SetScale(bbox.size);

    box_draw_objs.push_back({
        .transform = box_transform,
        .color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
    });
}

void ModelRenderScene::Step(VkCommandBuffer) {}

void ModelRenderScene::Clear() {
    for (auto& mesh : mesh_draw_objs) {
        mesh.mesh.indices.Destroy();
        mesh.mesh.vertices.Destroy();
    }
}

void ModelRenderScene::Reset() {}
}  // namespace vfs
