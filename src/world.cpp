#include "world.h"

#include <cstdint>
#include <glm/ext.hpp>
#include <random>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "gfx/vk_util.h"
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

    for (auto& bufs : frame_buffers) {
        bufs = {
            .position_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
            .predicted_position_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
            .velocity_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
            .density_buffer = CreateDataBuffer<glm::vec2>(gfx.GetCoreCtx(), n_particles),
        };

        bufs.position_buffer_addr =
            vk::util::GetBufferAddress(gfx.GetCoreCtx().device, bufs.position_buffer);
        bufs.predicted_position_buffer_addr =
            vk::util::GetBufferAddress(gfx.GetCoreCtx().device, bufs.predicted_position_buffer);
        bufs.velocity_buffer_addr =
            vk::util::GetBufferAddress(gfx.GetCoreCtx().device, bufs.velocity_buffer);
        bufs.density_buffer_addr =
            vk::util::GetBufferAddress(gfx.GetCoreCtx().device, bufs.density_buffer);
    }

    auto global_uniforms = GlobalUniformData{
        .g = 0.0f,
        .mass = 1.0f,
        .damping_factor = 0.05f,
        .target_density = 10.0f,
        .pressure_multiplier = 500.0f,
        .smoothing_radius = 0.35f,
        .box = box,
    };

    global_desc_manager.Init(gfx.GetCoreCtx());
    global_desc_manager.SetUniformData(global_uniforms);

    update_pos_pipeline.Init(gfx.GetCoreCtx(),
                             {
                                 .desc_manager = &global_desc_manager,
                                 .shader_path = "shaders/compiled/update_pos.comp.spv",
                                 .push_const_size = sizeof(ComputePushConstants),
                             });

    // boundaries_pipeline.Init(gfx.GetCoreCtx(),
    //                          {.desc_manager = &global_desc_manager,
    //                           .shader_path = "shaders/compiled/update_pos.comp.spv",
    //                           .push_const_size = sizeof(ComputePushConstants)});

    gfx::CPUMesh mesh;
    DrawCircleFill(mesh, glm::vec3(0.0f), 0.05f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 50);
    circle_mesh = gfx::UploadMesh(gfx, mesh);

    SetInitialData();
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
    std::vector<glm::vec2> initial_positions(n_particles);
    std::vector<glm::vec2> initial_velocities(n_particles);

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;
    for (int i = 0; i < n_particles; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);

        float x = box.x + r1 * box.z;
        float y = box.y + r2 * box.w;
        initial_positions[i] = glm::vec2{x, y};
        initial_velocities[i] = glm::vec2{-1.0f + 2.0f * r1, -1.0f + 2.0f * r2};  // REMOVE
    }

    for (auto& frame : frame_buffers) {
        SetDataVec(gfx, frame.position_buffer, initial_positions);
        SetDataVec(gfx, frame.predicted_position_buffer, initial_positions);
        SetDataVec(gfx, frame.velocity_buffer, initial_velocities);
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
    CPY_BUFFER(predicted_position_buffer);
    CPY_BUFFER(velocity_buffer);
    CPY_BUFFER(density_buffer);

}  // namespace vfs

void ComputeToComputePipelineBarrier(VkCommandBuffer cmd) {
    auto mem_barrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        // .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        // .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pMemoryBarriers = &mem_barrier,
        .memoryBarrierCount = 1,
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

void ComputeToGraphicsPipelineBarrier(VkCommandBuffer cmd) {
    auto mem_barrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pMemoryBarriers = &mem_barrier,
        .memoryBarrierCount = 1,
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

void World::RunSimulationStep(VkCommandBuffer cmd) {
    constexpr int iterations = 1;
    auto group_count = glm::ivec3{n_particles / 64 + 1, 1, 1};

    auto compute_constants = ComputePushConstants{
        .time = Platform::Info::GetTime(),
        .dt = 1 / (120.f * iterations),
        .n_particles = (uint32_t)n_particles,

        .positions = frame_buffers[current_frame].position_buffer_addr,
        .velocities = frame_buffers[current_frame].velocity_buffer_addr,
        .densities = frame_buffers[current_frame].density_buffer_addr,
    };

    for (int i = 0; i < iterations; i++) {
        update_pos_pipeline.Compute(cmd, gfx, group_count, &compute_constants);
        // ComputeToComputePipelineBarrier(cmd);
        // boundaries_pipeline.Compute(cmd, gfx, glm::ivec3(), &compute_constants);
        // TODO: write the others pipelines

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
    auto tr = glm::ortho(0.0f, (float)platform.GetConfig().w, 0.0f, (float)platform.GetConfig().h,
                         -1.0f, 1.0f);

    tr = glm::scale(tr, glm::vec3(1 / scale, 1 / scale, 1.0f));

    auto draw_push_constants = DrawPushConstants{
        .matrix = tr,
        .positions = frame_buffers[current_frame].position_buffer_addr,
    };

    renderer.Draw(gfx, cmd, circle_mesh, draw_push_constants, n_particles);

    gfx.EndFrame(cmd, renderer.GetDrawImage());

    current_frame = (current_frame + 1) % gfx::FRAME_COUNT;
}

void World::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    for (auto& frame : frame_buffers) {
        frame.position_buffer.Destroy();
        frame.predicted_position_buffer.Destroy();
        frame.velocity_buffer.Destroy();
        frame.density_buffer.Destroy();
    }

    global_desc_manager.Clear(gfx.GetCoreCtx());
    update_pos_pipeline.Clear(gfx.GetCoreCtx());
    // boundaries_pipeline.Clear(gfx.GetCoreCtx());
    gfx::DestroyMesh(gfx, circle_mesh);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs