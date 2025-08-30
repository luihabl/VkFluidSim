#include "world.h"

#include <cstdint>
#include <glm/ext.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <random>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "pipeline.h"
#include "platform.h"

namespace vfs {

namespace {
template <typename T>
gfx::Buffer CreateDataBuffer(const gfx::CoreCtx& ctx, size_t n) {
    const auto sz = sizeof(T) * n;
    return gfx::Buffer::Create(
        ctx, sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
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
        mesh.vertices.insert(mesh.vertices.end(), {{.pos = p0, .color = color},
                                                   {.pos = p1, .color = color},
                                                   {.pos = p2, .color = color}});
    }
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

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, ext.width, ext.height);

    SetBox(17, 9);

    InitSimulationPipelines();

    gfx::CPUMesh mesh;
    DrawCircleFill(mesh, glm::vec3(0.0f), 0.05f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 50);
    circle_mesh = gfx::UploadMesh(gfx, mesh);

    SetInitialData();

    sort.Init(gfx.GetCoreCtx());
    offset.Init(gfx.GetCoreCtx());
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

    const float smoothing_radius = 0.35f;
    auto global_uniforms = SimulationUniformData{
        .gravity = -12.0f,
        .damping_factor = 0.95f,
        .smoothing_radius = smoothing_radius,
        .target_density = 55.0f,
        .pressure_multiplier = 500.0f,
        .near_pressure_multiplier = 5.0f,
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

    global_desc_manager.Init(gfx.GetCoreCtx(), sizeof(SimulationUniformData));
    global_desc_manager.SetUniformData(&global_uniforms);

    simulation_pipelines.Init(gfx.GetCoreCtx(), &global_desc_manager);

    std::string prefix = "shaders/compiled/";
    sim_pos = simulation_pipelines.Add(prefix + "update_pos.comp.spv");
    sim_ext_forces = simulation_pipelines.Add(prefix + "update_external_forces.comp.spv");
    sim_reorder_copyback = simulation_pipelines.Add(prefix + "update_reorder_copyback.comp.spv");
    sim_reorder = simulation_pipelines.Add(prefix + "update_reorder.comp.spv");
    sim_density = simulation_pipelines.Add(prefix + "update_density.comp.spv");
    sim_pressure = simulation_pipelines.Add(prefix + "update_pressure_force.comp.spv");
    sim_viscosity = simulation_pipelines.Add(prefix + "update_viscosity.comp.spv");
    sim_spatial_hash = simulation_pipelines.Add(prefix + "update_spatial_hash.comp.spv");
}

void World::SetBox(float w, float h) {
    float win_w = scale * Platform::Info::GetConfig()->w;
    float win_h = scale * Platform::Info::GetConfig()->h;

    box.x = win_w / 2.0 - w / 2.0;
    box.y = win_h / 2.0 - h / 2.0;
    box.z = w;
    box.w = h;
}

void World::SetInitialData() {
    auto center = glm::vec2(box.x + box.z / 2.0f, box.y + box.w / 2.0f);

    auto size = glm::vec2(6.42, 4.39);
    auto spawn_region = glm::vec4(center.x - size.x / 2, center.y - size.y / 2, size.x, size.y);

    auto pos = SpawnParticlesInBox(spawn_region, n_particles);

    for (auto& frame : frame_buffers) {
        SetDataVec(gfx, frame.position_buffer, pos);
        SetDataVal(gfx, frame.velocity_buffer, glm::vec2(0.0f));
        SetDataVal(gfx, frame.density_buffer, glm::vec2(0.0f));
    }
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
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
    constexpr int iterations = 3;
    auto group_count = glm::ivec3{n_particles / 64 + 1, 1, 1};

    auto comp_consts = SimulationPushConstants{
        .time = Platform::Info::GetTime(),
        .dt = fixed_dt / (float)iterations,
        .n_particles = (uint32_t)n_particles,

        .positions = frame_buffers[current_frame].position_buffer.device_addr,
        .velocities = frame_buffers[current_frame].velocity_buffer.device_addr,
        .densities = frame_buffers[current_frame].density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(n_particles / 64 + 1, 1, 1);
    simulation_pipelines.UpdatePushConstants(comp_consts);

    for (int i = 0; i < iterations; i++) {
        simulation_pipelines.Run(sim_ext_forces, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        // RunSpatial
        simulation_pipelines.Run(sim_spatial_hash, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        sort.Run(cmd, gfx.GetCoreCtx(), spatial_indices, spatial_keys, n_particles - 1);
        ComputeToComputePipelineBarrier(cmd);

        offset.Run(cmd, gfx.GetCoreCtx(), true, spatial_keys, spatial_offsets);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_reorder, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_reorder_copyback, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_density, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_pressure, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_viscosity, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipelines.Run(sim_pos, cmd, n_groups);
        ComputeToComputePipelineBarrier(cmd);

        if (i < iterations - 1)
            ComputeToComputePipelineBarrier(cmd);
    }
}

void World::Update(Platform& platform) {
    auto cmd = gfx.BeginFrame();

    // Run compute step
    RunSimulationStep(cmd);
    ComputeToGraphicsPipelineBarrier(cmd);

    // NOTE: this is probably slow... rework this to avoid copy every frame
    CopyBuffersToNextFrame(cmd);

    // Run graphics step
    auto tr = glm::ortho(0.0f, (float)platform.GetConfig().w, (float)platform.GetConfig().h, 0.0f,
                         -1.0f, 1.0f);

    tr = glm::scale(tr, glm::vec3(1 / scale, 1 / scale, 1.0f));

    auto draw_push_constants = DrawPushConstants{
        .matrix = tr,
        .positions = frame_buffers[current_frame].position_buffer.device_addr,
        .velocities = frame_buffers[current_frame].velocity_buffer.device_addr,
    };

    renderer.Draw(gfx, cmd, circle_mesh, draw_push_constants, n_particles);

    gfx.EndFrame(cmd, renderer.GetDrawImage());

    current_frame = (current_frame + 1) % gfx::FRAME_COUNT;
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

    simulation_pipelines.Clear();
    global_desc_manager.Clear(gfx.GetCoreCtx());
    gfx::DestroyMesh(gfx, circle_mesh);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs
