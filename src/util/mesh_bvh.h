#pragma once

#include "gfx/mesh.h"

namespace vfs {
class MeshBVH {
public:
    struct Node {
        glm::vec3 aabb_min;
        glm::vec3 aabb_max;
        u32 triangle_start{0};
        u32 triangle_count{0};
        u32 child_a{0};
        u32 child_b{0};
    };

    void Init(gfx::CPUMesh* mesh);
    void Build();
    const std::vector<Node>& GetNodes() { return nodes; }

private:
    gfx::CPUMesh* mesh{nullptr};
    std::vector<Node> nodes;
    std::vector<u32> triangle_start_idx;
    std::vector<glm::vec3> triangle_centroids;

    void UpdateBounds(u32 node_idx);

    std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> GetTriangleAtIdx(u32 idx);
    const glm::vec3& GetCentroidAtIdx(u32 idx);

    void Split(u32 node_idx);

    struct Axis {
        u32 axis{0};  // 0: x, 1: y, 2: z
        float pos;
    };
    Axis ChooseSplitPosition(const Node& node);

    static constexpr u32 max_depth = 10;
};
}  // namespace vfs
