#pragma once

#include <cstdint>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

struct GlobalUniformData {
    float dt = 1.0f / 120.0f;
    float g = 0.0f;
    float mass = 1.0f;
    float damping_factor = 0.05f;
    float target_density = 10.0f;
    float pressure_multiplier = 500.0f;
    float smoothing_radius = 0.35f;
};

class SpriteDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        VkDeviceAddress vertex_buffer;
        VkDeviceAddress pos_buffer;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const gfx::Buffer& buf,
              const glm::mat4& transform);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class ComputePipeline {
public:
    struct DataPoint {
        float x;
        float v;
    };

    struct PushConstants {
        float time;
        float dt;
        unsigned data_buffer_size;
        VkDeviceAddress in_buf;
        VkDeviceAddress out_buf;
    };

    void Init(const gfx::CoreCtx& ctx);
    void Compute(VkCommandBuffer cmd,
                 gfx::Device& gfx,
                 const gfx::Buffer& in,
                 const gfx::Buffer& out);
    void Clear(const gfx::CoreCtx& ctx);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;

    void UpdateUniformBuffers();

    // Descriptor sets
    gfx::DescriptorPoolAlloc desc_pool;
    VkDescriptorSetLayout desc_layout;
    std::array<VkDescriptorSet, gfx::FRAME_COUNT> desc_sets;
    std::array<gfx::Buffer, gfx::FRAME_COUNT> uniform_buffers;
    GlobalUniformData uniform_constant_data;
    uint32_t current_frame{0};
};

}  // namespace vfs