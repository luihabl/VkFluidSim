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
        case DescriptorManager::DescType::Storage:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorManager::DescType::CombinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorManager::DescType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorManager::DescType::SampledImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
}

// VkBufferUsageFlags GetBufferUsage(DescriptorManager::DescType type) {
//     switch (type) {
//         case DescriptorManager::DescType::Uniform:
//             return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//             break;
//         case DescriptorManager::DescType::Storage:
//             return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//             break;
//         default:
//             // Invalid
//             return 0;
//     }
// }

void DescriptorManager::Init(const gfx::CoreCtx& ctx, std::span<DescriptorInfo> desc) {
    desc_info_ref = desc;

    desc_pool.Init(ctx,
                   {{
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_DESCRIPTOR_TYPE_SAMPLER,
                       VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                   }},
                   32, 32);

    auto layout_builder = gfx::DescriptorLayoutBuilder{};

    for (u32 i = 0; i < desc_info_ref.size(); i++) {
        layout_builder.Add(i, GetDescType(desc_info_ref[i].type));
    }

    desc_layout = layout_builder.Build(ctx, VK_SHADER_STAGE_ALL);

    desc_set = desc_pool.Alloc(ctx, desc_layout);
    std::vector<VkWriteDescriptorSet> write_desc_sets;
    for (u32 i = 0; i < desc_info_ref.size(); i++) {
        switch (desc_info_ref[i].type) {
            case DescType::Uniform:
            case DescType::Storage: {
                write_desc_sets.push_back(
                    vk::util::WriteDescriptorSet(desc_set, GetDescType(desc_info_ref[i].type), i,
                                                 &desc_info_ref[i].buffer.data_buffer.desc_info));
                break;
            }
            case DescType::CombinedImageSampler:
            case DescType::Sampler:
            case DescType::SampledImage: {
                auto info = VkDescriptorImageInfo{
                    .sampler = desc_info_ref[i].image.sampler,
                    .imageView = desc_info_ref[i].image.image.view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };

                write_desc_sets.push_back(vk::util::WriteDescriptorSet(
                    desc_set, GetDescType(desc_info_ref[i].type), i, &info));

                break;
            }
        }
    }

    vkUpdateDescriptorSets(ctx.device, write_desc_sets.size(), write_desc_sets.data(), 0, nullptr);
}

void DescriptorManager::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout, nullptr);
    desc_pool.Clear(ctx);
}

void DescriptorManager::SetUniformData(u32 id, const void* data) const {
    memcpy(desc_info_ref[id].buffer.data_buffer.Map(), data,
           desc_info_ref[id].buffer.data_buffer.size);
}

}  // namespace gfx
