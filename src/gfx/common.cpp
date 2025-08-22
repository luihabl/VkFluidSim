#include "common.h"

namespace gfx {

void* Buffer::GetMapped() {
    return mapped;
}

void* Buffer::Map() {
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
    };

    Buffer buffer{};
    buffer.size = (u32)size;
    buffer.allocator = ctx.allocator;
    VK_CHECK(vmaCreateBuffer(ctx.allocator, &buffer_info, &vma_alloc_info, &buffer.buffer,
                             &buffer.alloc, nullptr));
    return buffer;
}

void Buffer::Destroy() {
    Unmap();
    vmaDestroyBuffer(allocator, buffer, alloc);
}

}  // namespace gfx