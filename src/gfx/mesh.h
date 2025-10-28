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
    std::vector<Vertex> vertices;

    // The difference between indices and position_indices is that, the first will be a reference to
    // the actual unique vertices defining the primitives (triangles) to be rendered. The second is
    // a reference of unique vertex positions. This helps when processing the mesh with spatial
    // algorithms.
    std::vector<u32> indices;
    std::vector<u32> position_indices;
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
