#include "common.h"

namespace vfs {

void DrawCircleFill(gfx::CPUMesh& mesh, const glm::vec3& center, float radius, int steps) {
    float cx = center.x;
    float cy = center.y;

    for (int i = 0; i < steps; i++) {
        float angle0 = (float)i * 2 * M_PI / (float)steps;
        float angle1 = (float)(i + 1) * 2 * M_PI / (float)steps;

        auto p0 = glm::vec3(cx, cy, 0.0f);
        auto p1 = glm::vec3(cx + radius * sinf(angle0), cy + radius * cosf(angle0), 0.0f);
        auto p2 = glm::vec3(cx + radius * sinf(angle1), cy + radius * cosf(angle1), 0.0f);

        const unsigned int n = (unsigned int)mesh.vertices.size();
        mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1});
        mesh.vertices.insert(mesh.vertices.end(),
                             {{.pos = p0, .uv = {}}, {.pos = p1, .uv = {}}, {.pos = p2, .uv = {}}});
    }
}

void DrawQuad(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color) {
    const unsigned int n = (unsigned int)mesh.vertices.size();

    float hs = 0.5 * side;

    auto ll = glm::vec3(center.x - hs, center.y - hs, 0.0f);
    auto lr = glm::vec3(center.x + hs, center.y - hs, 0.0f);
    auto ur = glm::vec3(center.x + hs, center.y + hs, 0.0f);
    auto ul = glm::vec3(center.x - hs, center.y + hs, 0.0f);

    mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1, n + 0, n + 3, n + 2});
    mesh.vertices.push_back({.pos = ll, .uv = {0.0f, 0.0f}});
    mesh.vertices.push_back({.pos = lr, .uv = {1.0f, 0.0f}});
    mesh.vertices.push_back({.pos = ur, .uv = {1.0f, 1.0f}});
    mesh.vertices.push_back({.pos = ul, .uv = {0.0f, 1.0f}});
}

gfx::Image CreateDrawImage(const gfx::CoreCtx& ctx, u32 w, u32 h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

    VkImageUsageFlags usage{0};
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    bool use_mipmaps = false;

    return gfx::Image::Create(ctx, extent, format, usage, use_mipmaps);
}

gfx::Image CreateDepthImage(const gfx::CoreCtx& ctx, u32 w, u32 h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};
    VkFormat format = VK_FORMAT_D32_SFLOAT;

    VkImageUsageFlags usage{0};
    usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return gfx::Image::Create(ctx, extent, format, usage, false);
}
}  // namespace vfs
