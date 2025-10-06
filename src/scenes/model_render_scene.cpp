#include "model_render_scene.h"

#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "gui/common.h"
#include "platform.h"
#include "util/mesh_loader.h"

namespace vfs {
void ModelRenderScene::Init() {
    auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/box.obj").c_str());
    fmt::println("Number of shapes: {}", obj.size());
    fmt::println("First shape name: {}", obj.front().name);
    fmt::println("First shape number of vertices: {}", obj.front().vertices.size());
    fmt::println("First shape number of indices: {}", obj.front().indices.size());

    auto model_mesh = gfx::UploadMesh(gfx, obj.front());

    auto tr = gfx::Transform();

    mesh_draw_objs.push_back({
        .mesh = model_mesh,
        .transform = gfx::Transform{},
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
