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

struct ComputePushConstants {
    float time;
    float dt;
    unsigned n_particles;
    VkDeviceAddress positions;
    VkDeviceAddress velocities;
    VkDeviceAddress densities;
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

class DescriptorManager {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);

    void SetUniformData(const GlobalUniformData& data);

    gfx::DescriptorPoolAlloc desc_pool;
    VkDescriptorSetLayout desc_layout;
    VkDescriptorSet desc_set;
    gfx::Buffer global_constants_ubo;
    GlobalUniformData uniform_constant_data;
};

class ComputePipeline {
public:
    struct Config {
        uint32_t push_const_size;
        DescriptorManager* desc_manager;
        std::string shader_path;
    };

    void Init(const gfx::CoreCtx& ctx, const Config& config);
    void Compute(VkCommandBuffer cmd,
                 gfx::Device& gfx,
                 glm::ivec3 group_count,
                 void* push_constants = nullptr);
    void Clear(const gfx::CoreCtx& ctx);

private:
    Config config;
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

}  // namespace vfs