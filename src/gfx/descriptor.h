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
    enum class DescType { Uniform, Storage };
    struct DescData {
        u32 size{0};
        DescType type{DescType::Uniform};
    };

    void Init(const gfx::CoreCtx& ctx, const std::vector<DescData>& desc);
    void Clear(const gfx::CoreCtx& ctx);

    void SetUniformData(u32 id, const void* data) const;
    VkDescriptorSet Set() const { return desc_set; }
    VkDescriptorSetLayout Layout() const { return desc_layout; }

    gfx::DescriptorPoolAlloc desc_pool;

    VkDescriptorSet desc_set;
    VkDescriptorSetLayout desc_layout;

    std::vector<DescData> desc_data;
    std::vector<gfx::Buffer> desc_buffers;
};

}  // namespace gfx
