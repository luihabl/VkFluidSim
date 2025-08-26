#pragma once

#include <cstdint>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

struct GlobalUniformData {
    float g = 0.0f;
    float mass = 1.0f;
    float damping_factor = 0.05f;
    float target_density = 10.0f;
    float pressure_multiplier = 500.0f;
    float smoothing_radius = 0.35f;
    glm::vec4 box = {0, 0, 1, 1};
};

struct BufferUniformData {
    VkDeviceAddress positions;
    VkDeviceAddress predicted_positions;
    VkDeviceAddress velocities;
    VkDeviceAddress densities;
};

struct ComputePushConstants {
    float time;
    float dt;
    unsigned n_particles;
};

struct DrawPushConstants {
    glm::mat4 matrix;
    VkDeviceAddress positions;
};

class SpriteDrawPipeline {
public:
    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const DrawPushConstants& push_constants,
              uint32_t instances);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class ComputePipeline {
public:
    void Init(const gfx::CoreCtx& ctx, const char* shader_path);
    void SetUniformData(const GlobalUniformData& data);
    void SetBuffers(const std::vector<BufferUniformData>& data);

    void Compute(VkCommandBuffer cmd, gfx::Device& gfx, const ComputePushConstants& push_constants);
    void Clear(const gfx::CoreCtx& ctx);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;

    void UpdateUniformBuffers(uint32_t frame);

    // Descriptor sets
    gfx::DescriptorPoolAlloc desc_pool;
    VkDescriptorSetLayout desc_layout;

    struct FrameData {
        VkDescriptorSet desc_set;
        gfx::Buffer global_constants_ubo;
        gfx::Buffer buffers_ubo;
        bool should_update = false;

        GlobalUniformData uniform_constant_data;
        BufferUniformData buffer_uniform_data;
    };

    std::array<FrameData, gfx::FRAME_COUNT> frame_data;
    uint32_t current_frame{0};
};

}  // namespace vfs