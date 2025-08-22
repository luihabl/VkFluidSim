#include "mesh.h"

#include "gfx/common.h"

namespace gfx {
GPUMesh UploadMesh(const gfx::Device& gfx, const CPUMesh& mesh) {
    const size_t vertex_buf_size = mesh.vertices.size() * sizeof(Vertex);
    const size_t index_buf_size = mesh.indices.size() * sizeof(u32);

    GPUMesh gpu_mesh;

    gpu_mesh.index_count = mesh.indices.size();
    gpu_mesh.vertex_count = mesh.vertices.size();

    gpu_mesh.vertices =
        Buffer::Create(gfx.GetCoreCtx(), vertex_buf_size,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY);

    auto device_addr_info = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = gpu_mesh.vertices.buffer,
    };

    gpu_mesh.vertex_addr = vkGetBufferDeviceAddress(gfx.GetCoreCtx().device, &device_addr_info);

    gpu_mesh.indices =
        Buffer::Create(gfx.GetCoreCtx(), index_buf_size,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY);

    auto staging = Buffer::Create(gfx.GetCoreCtx(), vertex_buf_size + index_buf_size,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.Map();

    memcpy(data, mesh.vertices.data(), vertex_buf_size);
    memcpy((char*)data + vertex_buf_size, mesh.indices.data(), index_buf_size);

    gfx.ImmediateSubmit([&](VkCommandBuffer cmd) {
        auto vertex_copy = VkBufferCopy{
            .dstOffset = 0,
            .srcOffset = 0,
            .size = vertex_buf_size,
        };

        vkCmdCopyBuffer(cmd, staging.buffer, gpu_mesh.vertices.buffer, 1, &vertex_copy);

        auto index_copy = VkBufferCopy{
            .dstOffset = 0,
            .srcOffset = vertex_buf_size,
            .size = index_buf_size,
        };

        vkCmdCopyBuffer(cmd, staging.buffer, gpu_mesh.indices.buffer, 1, &index_copy);
    });

    staging.Destroy();

    return gpu_mesh;
}

void DestroyMesh(const gfx::Device& gfx, GPUMesh& mesh) {
    mesh.indices.Destroy();
    mesh.vertices.Destroy();
}
}  // namespace gfx