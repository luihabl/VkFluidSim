#include "world.h"

#include <glm/ext.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "imgui.h"
#include "pipeline.h"
#include "platform.h"

namespace {}

namespace vfs {

namespace {

void DrawCircleFill(gfx::CPUMesh& mesh,
                    const glm::vec3& center,
                    float radius,
                    const glm::vec4& color,
                    int steps) {
    float cx = center.x;
    float cy = center.y;

    for (int i = 0; i < steps; i++) {
        float angle0 = (float)i * 2 * M_PI / (float)steps;
        float angle1 = (float)(i + 1) * 2 * M_PI / (float)steps;

        auto p0 = glm::vec3(cx, cy, 0.0f);
        auto p1 = glm::vec3(cx + radius * sinf(angle0), cy + radius * cosf(angle0), 0.0f);
        auto p2 = glm::vec3(cx + radius * sinf(angle1), cy + radius * cosf(angle1), 0.0f);

        const unsigned int n = (unsigned int)mesh.vertices.size();
        mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1});
        mesh.vertices.insert(mesh.vertices.end(),
                             {{.pos = p0, .uv = {}}, {.pos = p1, .uv = {}}, {.pos = p2, .uv = {}}});
    }
}

void DrawSquare(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color) {
    const unsigned int n = (unsigned int)mesh.vertices.size();

    float hs = 0.5 * side;

    auto ll = glm::vec3(center.x - hs, center.y - hs, 0.0f);
    auto lr = glm::vec3(center.x + hs, center.y - hs, 0.0f);
    auto ur = glm::vec3(center.x + hs, center.y + hs, 0.0f);
    auto ul = glm::vec3(center.x - hs, center.y + hs, 0.0f);

    mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1, n + 0, n + 3, n + 2});
    mesh.vertices.push_back({.pos = ll, .uv = {0.0f, 0.0f}});
    mesh.vertices.push_back({.pos = lr, .uv = {1.0f, 0.0f}});
    mesh.vertices.push_back({.pos = ur, .uv = {1.0f, 1.0f}});
    mesh.vertices.push_back({.pos = ul, .uv = {0.0f, 1.0f}});
}

}  // namespace

void World::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim",
        .window = platform.GetWindow(),
        .validation_layers = true,
    });

    gpu_times.resize(6, 0);

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, ext.width, ext.height);
    ui.Init(gfx);

    simulation.Init(gfx.GetCoreCtx());

    gfx::CPUMesh mesh;
    DrawSquare(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    particle_mesh = gfx::UploadMesh(gfx, mesh);

    camera.GetTransform().scale = glm::vec3(65.0f);

    auto win_size = glm::vec2(Platform::Info::GetConfig()->w, Platform::Info::GetConfig()->h);
    auto box_size = simulation.GetBoundingBox().size / particle_scale;
    camera.GetTransform().position = -glm::vec3((win_size - box_size) / 2.0f, 0.0f);

    ResetSimulation();
}

void World::ResetSimulation() {
    auto box = simulation.GetBoundingBox();
    auto size = box.size / 3.0f;
    auto pos = box.pos + (box.size - size) / 2.0f;

    simulation.SetParticleInBox(gfx, {.size = size, .pos = pos});
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    ui.HandleEvent(e);

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World::Update(Platform& platform) {
    auto cmd = gfx.BeginFrame();
    gfx.SetTopTimestamp(cmd, 0);

    if (!paused) {
        simulation.Step(cmd, gfx.GetCoreCtx(), current_frame);
        ComputeToGraphicsPipelineBarrier(cmd);
    }

    gfx.SetBottomTimestamp(cmd, 1);

    const auto& buffers = simulation.GetFrameData(current_frame);
    auto draw_push_constants = DrawPushConstants{
        .matrix = camera.ViewProjectionMatrix(),
        .positions = buffers.position_buffer.device_addr,
        .velocities = buffers.velocity_buffer.device_addr,
    };

    renderer.Draw(gfx, cmd, particle_mesh, draw_push_constants,
                  simulation.GetParameters().n_particles);

    gfx.SetBottomTimestamp(cmd, 2);

    DrawUI(cmd);

    gfx.SetBottomTimestamp(cmd, 3);

    gfx.EndFrame(cmd, renderer.GetDrawImage());

    gpu_timestamps = gfx.GetTimestamps();

    current_frame = (current_frame + 1) % gfx::FRAME_COUNT;
}

void World::DrawUI(VkCommandBuffer cmd) {
    ui.BeginDraw(gfx, cmd, renderer.GetDrawImage());

    ImGui::Begin("Simulation control");

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(gfx.GetCoreCtx().chosen_gpu, &props);

    auto GetDelta = [this, &props](u32 id_start, u32 id_end) {
        return gpu_timestamps.size() > id_end
                   ? (float)(gpu_timestamps[id_end] - gpu_timestamps[id_start]) *
                         props.limits.timestampPeriod / 1000000.0f
                   : 0.0f;
    };

    constexpr float alpha = 0.05f;
    gpu_times[0] = (1 - alpha) * gpu_times[0] + alpha * GetDelta(0, 3);
    gpu_times[1] = (1 - alpha) * gpu_times[1] + alpha * GetDelta(0, 1);
    gpu_times[2] = (1 - alpha) * gpu_times[2] + alpha * GetDelta(1, 3);
    gpu_times[3] = (1 - alpha) * gpu_times[3] + alpha * GetDelta(1, 2);
    gpu_times[4] = (1 - alpha) * gpu_times[4] + alpha * GetDelta(2, 3);

    if (paused) {
        if (ImGui::Button("Run")) {
            paused = false;
        }
    } else {
        if (ImGui::Button("Pause")) {
            paused = true;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset")) {
        paused = true;
        ResetSimulation();
    }

    if (ImGui::CollapsingHeader("Stats")) {
        auto& io = ImGui::GetIO();
        ImGui::Text("FPS: %.2f", io.Framerate);

        ImGui::Text("Effective FPS: %.2f", 1000.0f / gpu_times[0]);
        ImGui::Text("Frame time: %.2f ms", gpu_times[0]);
        ImGui::Separator();

        ImGui::Text("Compute: %.2f ms", gpu_times[1]);
        ImGui::Text("All render: %.2f ms", gpu_times[2]);
        ImGui::Text("Particle render: %.2f ms", gpu_times[3]);
        ImGui::Text("UI render: %.2f ms", gpu_times[4]);
    }

    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat3("Position", &camera.GetTransform().position.x, -300.0f, 300.0f);

        float zoom = camera.GetTransform().scale.x;
        if (ImGui::SliderFloat("Scale", &zoom, 1.0f, 100.0f)) {
            camera.GetTransform().scale = glm::vec3(zoom);
        }
    }

    if (ImGui::CollapsingHeader("Parameters")) {
        glm::vec2 sz = simulation.GetBoundingBox().size;
        if (ImGui::SliderFloat2("Box size", &sz.x, 0.0f, 20.0f)) {
            simulation.SetBoundingBoxSize(sz.x, sz.y);
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Gravity", &simulation.UniformData().gravity, -25.0f, 25.0f)) {
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Wall damping factor", &simulation.UniformData().damping_factor,
                               0.0f, 1.0f)) {
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Pressure multiplier", &simulation.UniformData().pressure_multiplier,
                               0.0f, 2000.0f)) {
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Smoothing radius", &simulation.UniformData().smoothing_radius, 0.0f,
                               0.6f)) {
            simulation.UniformData().poly6_scale =
                4.0f /
                (glm::pi<float>() * (float)std::pow(simulation.UniformData().smoothing_radius, 8));
            simulation.UniformData().spiky_pow3_scale =
                10.0f /
                (glm::pi<float>() * (float)std::pow(simulation.UniformData().smoothing_radius, 5));
            simulation.UniformData().spiky_pow2_scale =
                6.0f /
                (glm::pi<float>() * (float)std::pow(simulation.UniformData().smoothing_radius, 4));
            simulation.UniformData().spiky_pow3_diff_scale =
                30.0f /
                (glm::pi<float>() * (float)std::pow(simulation.UniformData().smoothing_radius, 5));
            simulation.UniformData().spiky_pow2_diff_scale =
                12.0f /
                (glm::pi<float>() * (float)std::pow(simulation.UniformData().smoothing_radius, 4));
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Viscosity", &simulation.UniformData().viscosity_strenght, 0.0f,
                               1.0f)) {
            simulation.ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Target density", &simulation.UniformData().target_density, 0.0f,
                               100.0f)) {
            simulation.ScheduleUpdateUniforms();
        }
    }

    ImGui::End();

    ui.EndDraw(cmd);
}

void World::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    gfx::DestroyMesh(gfx, particle_mesh);
    renderer.Clear(gfx);
    simulation.Clear(gfx.GetCoreCtx());
    gfx.Clear();
}

}  // namespace vfs
