#include "world.h"

#include <cstdint>
#include <glm/ext.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <random>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "imgui.h"
#include "pipeline.h"
#include "platform.h"

namespace {
enum SimKernel : u32 {
    KernelUpdatePositions = 0,
    KernelExternalForces,
    KernelReorderCopyback,
    KernelReorder,
    KernelCalculateDensities,
    KernelCalculatePressureForces,
    KernelCalculateViscosityForces,
    KernelUpdateSpatialHash
};
}

namespace vfs {

namespace {
template <typename T>
gfx::Buffer CreateDataBuffer(const gfx::CoreCtx& ctx, size_t n) {
    const auto sz = sizeof(T) * n;
    return gfx::Buffer::Create(
        ctx, sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
}

template <typename T>
void SetDataVal(const gfx::Device& gfx, const gfx::Buffer& buf, const T& value) {
    auto staging = gfx::Buffer::Create(gfx.GetCoreCtx(), buf.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_MEMORY_USAGE_CPU_ONLY);
    void* data = staging.Map();

    size_t n = buf.size / sizeof(T);
    auto* dp = (T*)data;

    for (int i = 0; i < n; i++) {
        dp[i] = value;
    }

    gfx.ImmediateSubmit([&](VkCommandBuffer cmd) {
        auto cpy_info = VkBufferCopy{
            .dstOffset = 0,
            .srcOffset = 0,
            .size = buf.size,
        };

        vkCmdCopyBuffer(cmd, staging.buffer, buf.buffer, 1, &cpy_info);
    });

    staging.Destroy();
}

template <typename T>
void SetDataVec(const gfx::Device& gfx, const gfx::Buffer& buf, const std::vector<T>& value) {
    auto staging = gfx::Buffer::Create(gfx.GetCoreCtx(), buf.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_MEMORY_USAGE_CPU_ONLY);
    void* data = staging.Map();

    size_t n = buf.size / sizeof(T);
    auto* dp = (T*)data;

    for (int i = 0; i < n; i++) {
        dp[i] = value[i];
    }

    gfx.ImmediateSubmit([&](VkCommandBuffer cmd) {
        auto cpy_info = VkBufferCopy{
            .dstOffset = 0,
            .srcOffset = 0,
            .size = buf.size,
        };

        vkCmdCopyBuffer(cmd, staging.buffer, buf.buffer, 1, &cpy_info);
    });

    staging.Destroy();
}

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

std::vector<glm::vec2> SpawnParticlesInBox(glm::vec4 box, u32 count) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;

    auto p = std::vector<glm::vec2>(count);

    for (u32 i = 0; i < count; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);

        float x = box.x + r1 * box.z;
        float y = box.y + r2 * box.w;

        p[i] = glm::vec2{x, y};
    }

    return p;
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

    iterations = 3;
    time_scale = 1.0f;
    scale = 1.5e-2;
    n_particles = 16324;
    SetBox(17.1, 9.3);

    InitSimulationPipelines();

    gfx::CPUMesh mesh;
    DrawSquare(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    particle_mesh = gfx::UploadMesh(gfx, mesh);

    SetInitialData();

    sort.Init(gfx.GetCoreCtx());
    offset.Init(gfx.GetCoreCtx());
}

void World::UpdateUniforms() {
    global_desc_manager.SetUniformData(&sim_uniform_data);
}

void World::InitSimulationPipelines() {
    for (auto& bufs : frame_buffers) {
        bufs = {
            .position_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
            .velocity_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
            .density_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
        };
    }

    predicted_positions = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles);

    spatial_keys = CreateDataBuffer<u32>(gfx.GetCoreCtx(), n_particles);
    spatial_indices = CreateDataBuffer<u32>(gfx.GetCoreCtx(), n_particles);
    spatial_offsets = CreateDataBuffer<u32>(gfx.GetCoreCtx(), n_particles);

    sort_target_position = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles);
    sort_target_pred_position = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles);
    sort_target_velocity = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles);

    global_desc_manager.Init(gfx.GetCoreCtx(), sizeof(SimulationUniformData));
    global_desc_manager.SetUniformData(&sim_uniform_data);

    simulation_pipeline.Init(gfx.GetCoreCtx(),
                             {
                                 .desc_manager = &global_desc_manager,
                                 .push_const_size = sizeof(SimulationPushConstants),
                                 .shader_path = "shaders/compiled/simulation.slang.spv",
                                 .kernels =
                                     {
                                         "update_positions",
                                         "external_forces",
                                         "reorder_copyback",
                                         "reorder",
                                         "calculate_densities",
                                         "calculate_pressure_forces",
                                         "calculate_viscosity_forces",
                                         "update_spatial_hash",
                                     },
                             });
}

void World::SetBox(float w, float h) {
    float win_w = scale * Platform::Info::GetConfig()->w;
    float win_h = scale * Platform::Info::GetConfig()->h;

    box.x = win_w / 2.0 - w / 2.0;
    box.y = win_h / 2.0 - h / 2.0;
    box.z = w;
    box.w = h;
}

void World::ResetSimulation() {
    auto center = glm::vec2(box.x + box.z / 2.0f, box.y + box.w / 2.0f);

    auto size = glm::vec2(box.z / 3, box.w / 3);
    auto spawn_region = glm::vec4(center.x - size.x / 2, center.y - size.y / 2, size.x, size.y);

    auto pos = SpawnParticlesInBox(spawn_region, n_particles);

    for (auto& frame : frame_buffers) {
        SetDataVec(gfx, frame.position_buffer, pos);
        SetDataVal(gfx, frame.velocity_buffer, glm::vec2(0.0f));
        SetDataVal(gfx, frame.density_buffer, glm::vec2(0.0f));
    }
}

void World::SetInitialData() {
    const float smoothing_radius = 0.2;
    sim_uniform_data = SimulationUniformData{
        .gravity = -13.0f,
        .damping_factor = 0.5f,
        .smoothing_radius = smoothing_radius,
        .target_density = 234.0f,
        .pressure_multiplier = 225.0f,
        .near_pressure_multiplier = 18.0f,
        .viscosity_strenght = 0.03f,

        .box = box,

        .poly6_scale = 4.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 8)),
        .spiky_pow3_scale = 10.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow2_scale = 6.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 4)),
        .spiky_pow3_diff_scale = 30.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow2_diff_scale = 12.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 4)),

        .predicted_positions = predicted_positions.device_addr,

        .spatial_keys = spatial_keys.device_addr,
        .spatial_offsets = spatial_offsets.device_addr,
        .sorted_indices = spatial_indices.device_addr,

        .sort_target_positions = sort_target_position.device_addr,
        .sort_target_pred_positions = sort_target_pred_position.device_addr,
        .sort_target_velocities = sort_target_velocity.device_addr,
    };

    UpdateUniforms();

    ResetSimulation();
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    ui.HandleEvent(e);

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World::CopyBuffersToNextFrame(VkCommandBuffer cmd) {
    int next_frame = (current_frame + 1) % gfx::FRAME_COUNT;

#define CPY_BUFFER(name)                                                                     \
    {                                                                                        \
        auto cpy_info = VkBufferCopy{                                                        \
            .dstOffset = 0, .srcOffset = 0, .size = frame_buffers[current_frame].name.size}; \
        vkCmdCopyBuffer(cmd, frame_buffers[current_frame].name.buffer,                       \
                        frame_buffers[next_frame].name.buffer, 1, &cpy_info);                \
    }

    CPY_BUFFER(position_buffer);
    CPY_BUFFER(velocity_buffer);
    CPY_BUFFER(density_buffer);

}  // namespace vfs

void World::RunSimulationStep(VkCommandBuffer cmd) {
    auto comp_consts = SimulationPushConstants{
        .time = Platform::Info::GetTime(),
        .dt = time_scale * fixed_dt / (float)iterations,
        .n_particles = (uint32_t)n_particles,

        .positions = frame_buffers[current_frame].position_buffer.device_addr,
        .velocities = frame_buffers[current_frame].velocity_buffer.device_addr,
        .densities = frame_buffers[current_frame].density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(n_particles / 64 + 1, 1, 1);

    for (int i = 0; i < iterations; i++) {
        simulation_pipeline.Compute(cmd, KernelExternalForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // RunSpatial
        simulation_pipeline.Compute(cmd, KernelUpdateSpatialHash, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        sort.Run(cmd, gfx.GetCoreCtx(), spatial_indices, spatial_keys, n_particles - 1);
        ComputeToComputePipelineBarrier(cmd);

        offset.Run(cmd, gfx.GetCoreCtx(), true, spatial_keys, spatial_offsets);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorder, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorderCopyback, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculatePressureForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // simulation_pipeline.Compute(cmd, KernelCalculateViscosityForces, n_groups, &comp_consts);
        // ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);
    }
}

void World::Update(Platform& platform) {
    if (update_uniform_data) {
        UpdateUniforms();
        update_uniform_data = false;
    }

    auto cmd = gfx.BeginFrame();

    gfx.SetTopTimestamp(cmd, 0);

    if (!paused) {
        // Run compute step
        RunSimulationStep(cmd);
        ComputeToGraphicsPipelineBarrier(cmd);
    }

    // NOTE: this is probably slow... rework this to avoid copy every frame
    CopyBuffersToNextFrame(cmd);
    ComputeToComputePipelineBarrier(cmd);

    gfx.SetBottomTimestamp(cmd, 1);

    // Run graphics step
    auto tr = glm::ortho(0.0f, (float)platform.GetConfig().w, (float)platform.GetConfig().h, 0.0f,
                         -1.0f, 1.0f);

    tr = glm::scale(tr, glm::vec3(1 / scale, 1 / scale, 1.0f));

    auto draw_push_constants = DrawPushConstants{
        .matrix = tr,
        .positions = frame_buffers[current_frame].position_buffer.device_addr,
        .velocities = frame_buffers[current_frame].velocity_buffer.device_addr,
    };

    renderer.Draw(gfx, cmd, particle_mesh, draw_push_constants, n_particles);

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

    if (ImGui::CollapsingHeader("Parameters")) {
        glm::vec2 sz = {box.z, box.w};
        if (ImGui::SliderFloat2("Box size", &sz.x, 0.0f, 20.0f)) {
            SetBox(sz.x, sz.y);
            sim_uniform_data.box = box;
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Gravity", &sim_uniform_data.gravity, -25.0f, 25.0f)) {
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Wall damping factor", &sim_uniform_data.damping_factor, 0.0f,
                               1.0f)) {
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Pressure multiplier", &sim_uniform_data.pressure_multiplier, 0.0f,
                               2000.0f)) {
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Smoothing radius", &sim_uniform_data.smoothing_radius, 0.0f,
                               0.6f)) {
            sim_uniform_data.poly6_scale =
                4.0f / (glm::pi<float>() * (float)std::pow(sim_uniform_data.smoothing_radius, 8));
            sim_uniform_data.spiky_pow3_scale =
                10.0f / (glm::pi<float>() * (float)std::pow(sim_uniform_data.smoothing_radius, 5));
            sim_uniform_data.spiky_pow2_scale =
                6.0f / (glm::pi<float>() * (float)std::pow(sim_uniform_data.smoothing_radius, 4));
            sim_uniform_data.spiky_pow3_diff_scale =
                30.0f / (glm::pi<float>() * (float)std::pow(sim_uniform_data.smoothing_radius, 5));
            sim_uniform_data.spiky_pow2_diff_scale =
                12.0f / (glm::pi<float>() * (float)std::pow(sim_uniform_data.smoothing_radius, 4));
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Viscosity", &sim_uniform_data.viscosity_strenght, 0.0f, 1.0f)) {
            update_uniform_data = true;
        }

        if (ImGui::SliderFloat("Target density", &sim_uniform_data.target_density, 0.0f, 100.0f)) {
            update_uniform_data = true;
        }
    }

    ImGui::End();

    ui.EndDraw(cmd);
}

void World::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);

    predicted_positions.Destroy();

    spatial_keys.Destroy();
    spatial_indices.Destroy();
    spatial_offsets.Destroy();

    sort_target_position.Destroy();
    sort_target_pred_position.Destroy();
    sort_target_velocity.Destroy();

    sort.Clear(gfx.GetCoreCtx());
    offset.Clear(gfx.GetCoreCtx());

    for (auto& frame : frame_buffers) {
        frame.position_buffer.Destroy();
        frame.velocity_buffer.Destroy();
        frame.density_buffer.Destroy();
    }

    simulation_pipeline.Clear(gfx.GetCoreCtx());
    global_desc_manager.Clear(gfx.GetCoreCtx());
    gfx::DestroyMesh(gfx, particle_mesh);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs
