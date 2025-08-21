#include "descriptor.h"

#include "gfx/common.h"

namespace gfx {

void DescriptorPoolAlloc::Init(const CoreCtx& ctx,
                               std::span<const VkDescriptorType> descriptor_types,
                               u32 max_descriptors,
                               u32 max_sets) {
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto desc_type : descriptor_types) {
        pool_sizes.push_back(VkDescriptorPoolSize{
            .type = desc_type,
            .descriptorCount = max_descriptors,
        });
    }

    Init(ctx, pool_sizes, max_sets);
}

void DescriptorPoolAlloc::Init(const CoreCtx& ctx,
                               std::span<const VkDescriptorPoolSize> pool_sizes,
                               u32 max_sets) {
    auto pool_info = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = max_sets,
        .poolSizeCount = (uint32_t)pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(ctx.device, &pool_info, NULL, &pool));
}

VkDescriptorSet DescriptorPoolAlloc::Alloc(const CoreCtx& ctx, VkDescriptorSetLayout layout) {
    auto alloc_info = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptor_set;
    if (vkAllocateDescriptorSets(ctx.device, &alloc_info, &descriptor_set) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return descriptor_set;
}

void DescriptorPoolAlloc::Clear(const CoreCtx& ctx) {
    vkDestroyDescriptorPool(ctx.device, pool, nullptr);
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::Add(u32 binding, VkDescriptorType type) {
    bindings.push_back({
        .binding = binding,
        // TODO: check this count 1 here. Does it make any difference to have it in different
        // bindings?
        .descriptorCount = 1,
        .descriptorType = type,
    });

    return *this;
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(const CoreCtx& ctx,
                                                     VkShaderStageFlags stages,
                                                     void* p_next,
                                                     VkDescriptorSetLayoutCreateFlags flags) {
    for (auto& b : bindings) {
        b.stageFlags |= stages;
    }

    auto info = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pBindings = bindings.data(),
        .bindingCount = (uint32_t)bindings.size(),
        .flags = flags,
    };

    auto set = VkDescriptorSetLayout{};
    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &info, NULL, &set));

    return set;
}

void DescriptorLayoutBuilder::Clear() {}

}  // namespace gfx