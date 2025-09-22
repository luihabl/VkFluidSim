#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

class DescriptorManager {
public:
    void Init(const gfx::CoreCtx& ctx, u32 ubo_size);
    void Clear(const gfx::CoreCtx& ctx);

    void SetUniformData(void* data);

    gfx::DescriptorPoolAlloc desc_pool;
    VkDescriptorSetLayout desc_layout;
    VkDescriptorSet desc_set;
    gfx::Buffer global_constants_ubo;

    u32 size;
    std::vector<u8> uniform_constant_data;
};

class BoxDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        glm::vec4 color{1.0f};
    };

    void Init(const gfx::CoreCtx& ctx,
              VkFormat draw_img_format,
              VkFormat depth_img_format,
              bool draw_3d = false);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const PushConstants& push_constants);

private:
    u32 n_draw_vertices{8};
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class ParticleDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 matrix;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const PushConstants& push_constants,
              uint32_t instances);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class Particle3DDrawPipeline {
public:
    struct PushConstants {
        glm::mat4 model_view;
        glm::mat4 proj;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
    };

    void Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format, VkFormat depth_img_format);
    void Clear(const gfx::CoreCtx& ctx);
    void Draw(VkCommandBuffer cmd,
              gfx::Device& gfx,
              const gfx::Image& draw_img,
              const gfx::GPUMesh& mesh,
              const PushConstants& push_constants,
              uint32_t instances);

private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

class ComputePipeline {
public:
    struct Config {
        uint32_t push_const_size{0};
        DescriptorManager* desc_manager{nullptr};
        std::string shader_path{""};
        std::vector<std::string> kernels{{"main"}};
    };

    void Init(const gfx::CoreCtx& ctx, const Config& config);
    void Compute(VkCommandBuffer cmd,
                 u32 kernel_id,
                 glm::ivec3 group_count,
                 void* push_constants = nullptr);
    void Clear(const gfx::CoreCtx& ctx);
    u32 FindKernelId(const std::string& entry_point);

private:
    Config config;
    std::unordered_map<std::string, u32> ids;
    std::vector<VkPipeline> pipelines;
    VkPipelineLayout layout;
};

void ComputeToComputePipelineBarrier(VkCommandBuffer cmd);
void ComputeToGraphicsPipelineBarrier(VkCommandBuffer cmd);

}  // namespace vfs
