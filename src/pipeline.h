#pragma once

#include <cstdint>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"

namespace vfs {

struct DrawPushConstants {
    glm::mat4 matrix;
    VkDeviceAddress positions;
    VkDeviceAddress velocities;
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

class ComputePipeline {
public:
    struct Config {
        uint32_t push_const_size;
        DescriptorManager* desc_manager;
        std::string shader_path;
    };

    void Init(const gfx::CoreCtx& ctx, const Config& config);
    void Compute(VkCommandBuffer cmd, glm::ivec3 group_count, void* push_constants = nullptr);
    void Clear(const gfx::CoreCtx& ctx);

private:
    Config config;
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

template <typename PushConstantType>
class ComputePipelineSet {
public:
    void Init(const gfx::CoreCtx& context, DescriptorManager* desc_manager = nullptr) {
        ctx = context;
        descriptor_manager = desc_manager;
    }

    u32 Add(const std::string& shader_path) {
        pipelines.push_back({});
        pipelines.back().Init(ctx, {.push_const_size = sizeof(PushConstantType),
                                    .desc_manager = descriptor_manager,
                                    .shader_path = shader_path});
        return pipelines.size() - 1;
    }

    ComputePipeline& Get(u32 id) { return pipelines.at(id); }

    void Run(u32 id,
             VkCommandBuffer cmd,
             glm::ivec3 group_count,
             PushConstantType* push_consts = nullptr) {
        if (push_consts) {
            Get(id).Compute(cmd, group_count, push_consts);
        } else {
            Get(id).Compute(cmd, group_count, &push_constants);
        }
    }

    void Clear() {
        for (auto& p : pipelines) {
            p.Clear(ctx);
        }
    }

    void UpdatePushConstants(const PushConstantType& pc) { push_constants = pc; }

private:
    DescriptorManager* descriptor_manager = nullptr;
    PushConstantType push_constants;
    gfx::CoreCtx ctx;
    std::vector<ComputePipeline> pipelines;
};

void ComputeToComputePipelineBarrier(VkCommandBuffer cmd);
void ComputeToGraphicsPipelineBarrier(VkCommandBuffer cmd);

}  // namespace vfs
