#pragma once

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "common.h"
#include "gfx/gfx.h"
#include "gfx/transform.h"

namespace gfx {

struct BoundingBox {
    glm::vec3 size{0};
    glm::vec3 pos{0};
};

// TODO: change this later
struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct CPUMesh {
    std::string name;
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
};

struct GPUMesh {
    Buffer indices;
    u32 index_count{0};

    Buffer vertices;
    u32 vertex_count{0};

    VkDeviceAddress vertex_addr;
};

struct MeshDrawObj {
    gfx::Transform transform;
    gfx::GPUMesh mesh;
};

GPUMesh UploadMesh(const gfx::Device& gfx, const CPUMesh& mesh);
void DestroyMesh(const gfx::Device& gfx, GPUMesh& mesh);

}  // namespace gfx
