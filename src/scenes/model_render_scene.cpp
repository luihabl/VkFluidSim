#include "model_render_scene.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "imgui.h"
#include "platform.h"
#include "scenes/scene.h"
#include "util/geometry.h"
#include "util/mesh_bvh.h"
#include "util/mesh_loader.h"

namespace vfs {

void ModelRenderScene::Init() {
    {
        auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/box.obj").c_str());
        box_mesh = std::move(obj[0]);
        box_draw_obj = {
            .mesh = gfx::UploadMesh(gfx, box_mesh),
            .transform = gfx::Transform{},
        };
    }

    {
        auto obj = LoadObjMesh(Platform::Info::ResourcePath("models/suzanne.obj").c_str());
        model_mesh = std::move(obj[0]);
        model_draw_obj = {
            .mesh = gfx::UploadMesh(gfx, model_mesh),
            .transform = gfx::Transform{},
        };
    }

    model_bvh.Init(model_mesh, MeshBVH::SplitType::SurfaceAreaHeuristics);
    model_bvh.Build();

    fmt::println("Number of BVH nodes: {}", model_bvh.GetNodes().size());
    // u32 leaves = 0;
    // u32 triangles = 0;

    // for (u32 i = 0; i < bvh.GetNodes().size(); i++) {
    //     const auto& n = bvh.GetNodes()[i];
    //     if (n.child_a == 0) {
    //         leaves++;
    //         triangles += n.triangle_count;
    //     }
    // }

    // fmt::println("Number of leaves: {}, number of triangles: {}", leaves, triangles);

    SetQueryPoint(glm::vec3(0.5f, 1.2f, 0.8f));

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

        ImGui::Text("Closest point: %.2f %.2f %.2f", query_result.closest_point.x,
                    query_result.closest_point.y, query_result.closest_point.z);
        ImGui::Text("Distance: %.2f", std::sqrt(query_result.min_distance_sq));
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

    query_result = model_bvh.QueryClosestPoint(query_point);

    point_transform.SetPosition(query_result.closest_point);

    box_draw_objs[1] = {
        .transform = point_transform,
        .color = glm::vec4(1.0, 0.0, 0.0, 1.0),
    };

    // for (u32 i = 0; i < model_bvh.GetNodes().size(); i++) {
    //     auto& node = model_bvh.GetNodes()[i];

    //     if (node.child_a != 0)
    //         continue;

    //     auto dist = DistancePointToAABB(point, node.box.pos_min, node.box.pos_max);

    //     auto t0 = model_bvh.GetTriangleInfo()[node.triangle_start];
    //     auto t1 = model_bvh.GetTriangleInfo()[node.triangle_start + 1];

    //     auto t = gfx::Transform{};
    //     auto size = (node.box.pos_max - node.box.pos_min);

    //     t.SetPosition(node.box.pos_min + size / 2);
    //     t.SetScale(size * 1.05);

    //     box_draw_objs.push_back({
    //         .transform = t,
    //         .color = glm::vec4((i == query_result.node_idx), 0.0, 1.0, 1.0),
    //     });
    // }
}

void ModelRenderScene::Clear() {
    model_draw_obj.mesh.indices.Destroy();
    model_draw_obj.mesh.vertices.Destroy();

    box_draw_obj.mesh.indices.Destroy();
    box_draw_obj.mesh.vertices.Destroy();

    mesh_pipeline.Clear(gfx.GetCoreCtx());

    vkDestroySampler(gfx.GetCoreCtx().device, sdf_sampler, nullptr);
}

namespace {
u32 GetIndex3D(const glm::uvec3& size, const glm::uvec3& idx) {
    return idx.z + size.z * idx.y + size.z * size.y * idx.x;
}
}  // namespace

void ModelRenderScene::GenerateSDF() {
    sdf_n_cells = {20, 20, 20};
    sdf_grid.resize(sdf_n_cells.x * sdf_n_cells.y * sdf_n_cells.z, 0.0f);
    sdf_max_pos = {1, 1, 1};
    sdf_min_pos = {-1, -1, -1};

    auto step = (sdf_max_pos - sdf_min_pos) / ((glm::vec3)sdf_n_cells - 1.0f);

    for (u32 i = 0; i < sdf_n_cells.x; i++) {
        for (u32 j = 0; j < sdf_n_cells.y; j++) {
            for (u32 k = 0; k < sdf_n_cells.z; k++) {
                auto pos = step * glm::vec3(i, j, k) + sdf_min_pos;

                auto res = model_bvh.QueryClosestPoint(pos);
                sdf_grid[GetIndex3D(sdf_n_cells, {i, j, k})] = res.min_distance_sq;
            }
        }
    }

    // sdf_buffer = gfx::Buffer::Create(gfx.GetCoreCtx(), sdf_grid.size(), VkBufferUsageFlags
    // usage);
}

void ModelRenderScene::InitCustomDraw(VkFormat draw_img_fmt, VkFormat depth_img_format) {
    auto img = gfx::Image::Create(
        gfx.GetCoreCtx(),
        {.width = (u32)sdf_n_cells.x, .height = (u32)sdf_n_cells.y, .depth = (u32)sdf_n_cells.z},
        VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    gfx.SetImageData(img, sdf_grid.data(), sizeof(float));

    auto sampler_create_info = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };

    vkCreateSampler(gfx.GetCoreCtx().device, &sampler_create_info, nullptr, &sdf_sampler);

    desc_info.push_back({
        .type = gfx::DescriptorManager::DescType::CombinedImageSampler,
        .image =
            {
                .image = img,
                .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .sampler = sdf_sampler,
            },
    });

    descriptor_manager.Init(gfx.GetCoreCtx(), desc_info);

    mesh_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
    raymarch_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format,
                           descriptor_manager.Set(), descriptor_manager.Layout());
    box_pipeline.Init(gfx.GetCoreCtx(), draw_img_fmt, depth_img_format);
}

void ModelRenderScene::CustomDraw(VkCommandBuffer cmd,
                                  const gfx::Image& draw_img,
                                  const gfx::Camera& camera) {
    // for (const auto& mesh : mesh_draw_objs) {
    //     // mesh_pipeline.Draw(cmd, gfx, draw_img, mesh, camera);
    //     raymarch_pipeline.Draw(cmd, gfx, sdf_buffer, sdf_n_cells, draw_img, mesh, camera);
    // }

    // mesh_pipeline.Draw(cmd, gfx, draw_img, model_draw_obj, camera);
    raymarch_pipeline.Draw(cmd, gfx, sdf_max_pos - sdf_min_pos, draw_img, box_draw_obj, camera);
}

void ModelRenderScene::Reset() {}
}  // namespace vfs
