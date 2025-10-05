#include "model_render_scene.h"

#include "gfx/mesh.h"
#include "models/model.h"
#include "platform.h"
#include "util/mesh_loader.h"

namespace vfs {
void ModelRenderScene::Init() {
    auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/box.obj").c_str());
    fmt::println("Number of shapes: {}", obj.size());
    fmt::println("First shape name: {}", obj.front().name);
    fmt::println("First shape number of vertices: {}", obj.front().vertices.size());
    fmt::println("First shape number of indices: {}", obj.front().indices.size());

    model_mesh = gfx::UploadMesh(gfx, obj.front());
}

void ModelRenderScene::Step(VkCommandBuffer) {}

void ModelRenderScene::Clear() {
    model_mesh.vertices.Destroy();
    model_mesh.indices.Destroy();
}

void ModelRenderScene::Reset() {}
}  // namespace vfs
