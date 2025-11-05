#include "model_render_scene.h"

#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/mesh.h"
#include "gfx/transform.h"
#include "imgui.h"
#include "platform.h"
#include "scenes/scene.h"
#include "simulation.h"
#include "util/gaussian_quadrature.h"
#include "util/geometry.h"
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

    // sdf_max_pos = {1, 1, 1};
    // sdf_min_pos = {-1, -1, -1};

    // auto box = gfx::BoundingBox{
    //     .pos = glm::vec3(-1.0f),
    //     .size = glm::vec3(2.0f),
    // };

    sdf_n_cells = glm::uvec3(20);

    model_sdf.Init(model_mesh, sdf_n_cells, {}, 0.05);

    model_sdf.Build();

    // sdf_grid = model_sdf.GetSDF();
    sdf_grid.resize(model_sdf.GetSDF().size());
    GenerateVolumeMap();

    for (u32 i = 0; i < sdf_grid.size(); i++) {
        // sdf_grid[i] = (f32)model_sdf.GetSDF()[i];
        sdf_grid[i] = (f32)volume_map.GetGrid()[i];
    }

    // sdf_buffer = gfx::Buffer::Create(gfx.GetCoreCtx(), sdf_grid.size(), VkBufferUsageFlags
    // usage);

    // model_bvh.Init(model_mesh, MeshBVH::SplitType::SurfaceAreaHeuristics);
    // model_bvh.Build();

    // fmt::println("Number of BVH nodes: {}", model_bvh.GetNodes().size());
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

    SetQueryPoint(glm::vec3(0.780, -0.470, -0.130));

    // GenerateSDF();
}

void ModelRenderScene::Step(VkCommandBuffer cmd) {}

void ModelRenderScene::DrawDebugUI() {
    SceneBase::DrawDebugUI();

    if (ImGui::CollapsingHeader("Model render scene")) {
        glm::vec3 p = query_point;
        if (ImGui::DragFloat3("Query point", glm::value_ptr(p), sdf_tolerance)) {
            SetQueryPoint(p);
        }

        ImGui::Text("Closest point: %.2f %.2f %.2f", signed_distance.query_result.closest_point.x,
                    signed_distance.query_result.closest_point.y,
                    signed_distance.query_result.closest_point.z);

        ImGui::Text("Query distance: %.2f", signed_distance.signed_distance);

        auto interpolated_distance = model_sdf.Interpolate(query_point);
        ImGui::Text("Interpolation distance: %.2f", interpolated_distance);
    }
}

void ModelRenderScene::SetQueryPoint(const glm::vec3& point) {
    query_point = point;
    box_draw_objs.clear();

    auto point_transform = gfx::Transform{};
    auto scale = glm::vec3(0.02f);
    point_transform.SetScale(scale);
    point_transform.SetPosition(query_point);

    signed_distance =
        SignedDistanceToMesh(model_sdf.GetBVH(), model_sdf.GetPseudonormals(), query_point);
    signed_distance.signed_distance -= sdf_tolerance;

    auto color = glm::vec4(1.0, 0.0, 0.0, 1.0);
    if (signed_distance.signed_distance < 0) {
        color = glm::vec4(0.0, 0.0, 1.0, 1.0);
    }

    box_draw_objs.push_back({
        .transform = point_transform,
        .color = color,
        .hidden = false,
    });

    color = glm::vec4(1.0, 0.0, 0.0, 1.0),
    point_transform.SetPosition(signed_distance.query_result.closest_point);

    box_draw_objs.push_back({
        .transform = point_transform,
        .color = color,
        .hidden = false,
    });

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

void ModelRenderScene::GenerateVolumeMap() {
    // model_bvh.Init(model_mesh);
    // model_pseudonormals.Init(model_mesh);

    // model_discrete_grid.Init({20, 20, 20}, const AABB& domain,
    //                          std::function<double(const glm::dvec3&)> func)

    fmt::println("Generating volume map");

    auto& sim = Simulation::Get();
    const auto radius = sim.GetGlobalParameters().smooth_radius;
    auto sdf_box = model_sdf.GetBox();
    const auto domain = AABB{.pos_min = sdf_box.pos, .pos_max = sdf_box.pos + sdf_box.size};

    // TODO: move this to a separate util file
    auto cubic_kernel_w = [radius](double r) {
        const auto k = 8.0 / (M_PI * radius * radius * radius);

        double res = 0.0;
        const double q = r / radius;
        if (q <= 1.0) {
            if (q <= 0.5) {
                const double q2 = q * q;
                const double q3 = q2 * q;
                res = k * (6.0 * q3 - 6.0 * q2 + 1.0);
            } else {
                res = k * (2.0 * pow(1.0 - q, 3.0));
            }
        }
        return res;
    };

    auto cubic_kernel_w_zero = cubic_kernel_w(0);

    auto volume_map_func = [&](const glm::vec3& x) -> double {
        const double dist = model_sdf.Interpolate(x);
        // const double dist =
        //     SignedDistanceToMesh(model_sdf.GetBVH(), model_sdf.GetPseudonormals(), x)
        //         .signed_distance;

        if (dist > 2.0 * radius) {
            return 0.0;
        }

        auto integrand = [&](const glm::vec3& xi) -> double {
            if (glm::length2(xi) > radius * radius) {
                return 0.0;
            }

            auto dist_i = model_sdf.Interpolate(x + xi);
            // auto dist_i =
            //     SignedDistanceToMesh(model_sdf.GetBVH(), model_sdf.GetPseudonormals(), x + xi)
            //         .signed_distance;

            if (dist_i <= 0.0) {
                return 1.0;
            }

            else if (dist_i < radius) {
                return cubic_kernel_w(dist_i) / cubic_kernel_w_zero;
            }

            else {
                return 0;
            }
        };

        return 0.8 * GaussLegendreQuadrature3D(domain, 16, integrand);
    };

    volume_map.Init(sdf_n_cells, domain, volume_map_func);

    fmt::println("Done");
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

    raymarch_pipeline.Draw(cmd, gfx, model_sdf.GetBox().size, draw_img, box_draw_obj, camera);
    // mesh_pipeline.Draw(cmd, gfx, draw_img, model_draw_obj, camera);
}

void ModelRenderScene::Reset() {}
}  // namespace vfs
