#include "common.h"

#include "gfx/vk_util.h"

namespace gfx {

void* Buffer::GetMapped() {
    return mapped;
}

void* Buffer::Map() const {
    if (mapped)
        return mapped;

    vmaMapMemory(allocator, alloc, &mapped);
    return mapped;
}
void Buffer::Unmap() {
    if (mapped) {
        vmaUnmapMemory(allocator, alloc);
        mapped = nullptr;
    }
}

Buffer Buffer::Create(const CoreCtx& ctx,
                      size_t size,
                      VkBufferUsageFlags usage,
                      VmaMemoryUsage mem_usage) {
    auto buffer_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = size,
        .usage = usage,
    };

    auto vma_alloc_info = VmaAllocationCreateInfo{
        .usage = mem_usage,
        // TODO: check if it's necessary to use VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };

    Buffer buffer{};
    buffer.size = (u32)size;
    buffer.allocator = ctx.allocator;
    VK_CHECK(vmaCreateBuffer(ctx.allocator, &buffer_info, &vma_alloc_info, &buffer.buffer,
                             &buffer.alloc, nullptr));

    buffer.desc_info = {
        .buffer = buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        buffer.device_addr = vk::util::GetBufferAddress(ctx.device, buffer);
    }

    return buffer;
}

void Buffer::SetDescriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
    desc_info = {
        .buffer = buffer,
        .offset = offset,
        .range = size,
    };
}

void Buffer::Destroy() {
    if (buffer) {
        Unmap();
        vmaDestroyBuffer(allocator, buffer, alloc);
    }
}

Image Image::Create(const CoreCtx& ctx,
                    VkExtent3D size,
                    VkFormat format,
                    VkImageUsageFlags usage,
                    bool mip) {
    auto img = Image{
        .format = format,
        .extent = size,
        .device = ctx.device,
        .allocator = ctx.allocator,
    };

    u8 dims = 1;
    if (size.height > 1) {
        dims = 2;
        if (size.depth > 1) {
            dims = 3;
        }
    }

    auto img_info = vk::util::ImageCreateInfo(format, usage, size, dims);

    if (mip) {
        // TODO: check this
        img_info.mipLevels = (uint32_t)std::floor(std::log2(std::max(size.width, size.height))) + 1;
    }

    auto alloc_info = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_CHECK(
        vmaCreateImage(ctx.allocator, &img_info, &alloc_info, &img.image, &img.allocation, NULL));

    auto aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto view_info = vk::util::ImageViewCreateInfo(format, img.image, aspect_flag, dims);
    view_info.subresourceRange.levelCount = img_info.mipLevels;
    VK_CHECK(vkCreateImageView(ctx.device, &view_info, NULL, &img.view));

    return img;
}

void Image::Destroy() {
    if (image != VK_NULL_HANDLE) {
        vkDestroyImageView(device, view, NULL);
        vmaDestroyImage(allocator, image, allocation);
        image = VK_NULL_HANDLE;
    }
}
}  // namespace gfx
