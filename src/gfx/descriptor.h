#pragma once
#include <span>

#include "common.h"

namespace gfx {

class DescriptorPoolAlloc {
public:
    void Init(const CoreCtx& ctx,
              std::span<const VkDescriptorType> descriptor_types,
              u32 max_descriptors,
              u32 max_sets);

    void Init(const CoreCtx& ctx, std::span<const VkDescriptorPoolSize> pool_sizes, u32 max_sets);

    VkDescriptorSet Alloc(const CoreCtx& ctx, VkDescriptorSetLayout layout);

    void Clear(const CoreCtx& ctx);

private:
    VkDescriptorPool pool;
};

class DescriptorLayoutBuilder {
public:
    DescriptorLayoutBuilder& Add(u32 binding, VkDescriptorType type);
    VkDescriptorSetLayout Build(const CoreCtx& ctx,
                                VkShaderStageFlags stages,
                                void* p_next = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
    void Clear();

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
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

}  // namespace gfx
