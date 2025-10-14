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

    struct TriangleInfo {
        u32 vertex_start_idx;
        glm::vec3 centroid;
    };

    enum class SplitType {
        Midplane,
        SurfaceAreaHeuristics,
    };

    void Init(gfx::CPUMesh* mesh, SplitType split = SplitType::Midplane);

    void Build();
    const std::vector<Node>& GetNodes() { return nodes; }
    const std::vector<TriangleInfo>& GetTriangleInfo() { return triangles; }

private:
    struct Axis {
        u32 axis{0};  // 0: x, 1: y, 2: z
        float pos;
    };

    SplitType split_type{SplitType::Midplane};
    gfx::CPUMesh* mesh{nullptr};
    std::vector<Node> nodes;
    std::vector<TriangleInfo> triangles;
    static constexpr u32 max_depth = 10;

    void UpdateBounds(u32 node_idx);
    std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> GetTriangleAtIdx(u32 idx);
    const glm::vec3& GetCentroidAtIdx(u32 idx);
    void Split(u32 node_idx);
    Axis ChooseSplitPosition(const Node& node);
};
}  // namespace vfs
