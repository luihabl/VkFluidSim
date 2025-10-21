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
    enum class DescType {
        Uniform,
        Storage,
        CombinedImageSampler,
        Sampler,
        SampledImage,
    };

    struct BufferData {
        gfx::Buffer data_buffer;
    };

    struct ImageData {
        gfx::Image image;
        VkSampler sampler;
        VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    };

    struct DescriptorInfo {
        DescType type{DescType::Uniform};
        BufferData buffer;
        ImageData image;
    };

    void Init(const gfx::CoreCtx& ctx, std::span<DescriptorInfo> desc);
    void Clear(const gfx::CoreCtx& ctx);

    void SetUniformData(u32 id, const void* data) const;
    VkDescriptorSet Set() const { return desc_set; }
    VkDescriptorSetLayout Layout() const { return desc_layout; }

    gfx::DescriptorPoolAlloc desc_pool;

    VkDescriptorSet desc_set;
    VkDescriptorSetLayout desc_layout;

    std::span<DescriptorInfo> desc_info_ref;

    static DescriptorInfo CreateUniformInfo(const gfx::CoreCtx& ctx, u32 size) {
        return {
            .type = DescType::Uniform,
            .buffer = {.data_buffer =
                           gfx::Buffer::Create(ctx, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)},
        };
    }

    static DescriptorInfo CreateStorageBuffer(const gfx::CoreCtx& ctx, u32 size) {
        return {
            .type = DescType::Storage,
            .buffer = {.data_buffer =
                           gfx::Buffer::Create(ctx, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)},
        };
    }
};

}  // namespace gfx
