#include "descriptor.h"

#include "gfx/common.h"
#include "gfx/vk_util.h"

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

VkDescriptorType GetDescType(DescriptorManager::DescType type) {
    switch (type) {
        case DescriptorManager::DescType::Uniform:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case DescriptorManager::DescType::Storage:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
    }
}

VkBufferUsageFlags GetBufferUsage(DescriptorManager::DescType type) {
    switch (type) {
        case DescriptorManager::DescType::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case DescriptorManager::DescType::Storage:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
    }
}

void DescriptorManager::Init(const gfx::CoreCtx& ctx, const std::vector<DescData>& desc) {
    desc_data = desc;

    desc_pool.Init(ctx,
                   {{
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   }},
                   256, 256);

    auto layout_builder = gfx::DescriptorLayoutBuilder{};

    for (u32 i = 0; i < desc_data.size(); i++) {
        layout_builder.Add(i, GetDescType(desc_data[i].type));
        desc_buffers.push_back(
            gfx::Buffer::Create(ctx, desc_data[i].size, GetBufferUsage(desc_data[i].type)));
    }

    desc_layout = layout_builder.Build(ctx, VK_SHADER_STAGE_ALL);

    desc_set = desc_pool.Alloc(ctx, desc_layout);
    std::vector<VkWriteDescriptorSet> write_desc_sets;
    for (u32 i = 0; i < desc_data.size(); i++) {
        write_desc_sets.push_back(vk::util::WriteDescriptorSet(
            desc_set, GetDescType(desc_data[i].type), i, &desc_buffers[i].desc_info));
    }

    vkUpdateDescriptorSets(ctx.device, write_desc_sets.size(), write_desc_sets.data(), 0, nullptr);
}

void DescriptorManager::Clear(const gfx::CoreCtx& ctx) {
    for (auto& buf : desc_buffers) {
        buf.Destroy();
    }
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout, nullptr);
    desc_pool.Clear(ctx);
}

void DescriptorManager::SetUniformData(u32 id, const void* data) const {
    memcpy(desc_buffers[id].Map(), data, desc_data[id].size);
}

}  // namespace gfx
