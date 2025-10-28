#pragma once

#include "gfx/mesh.h"
#include "util/geometry.h"

namespace vfs {
class MeshBVH {
public:
    struct Node {
        AABB box;
        u32 triangle_start{0};
        u32 triangle_count{0};
        u32 child_a{0};
        u32 child_b{0};

        float Cost() const;
    };

    enum class SplitType {
        Midplane,
        SurfaceAreaHeuristics,
        BinSplit,
    };

    void Init(const gfx::CPUMesh& mesh, SplitType split = SplitType::BinSplit);

    void Build();
    const std::vector<Node>& GetNodes() { return nodes; }
    const std::vector<TriangleInfo>& GetTriangleInfo() { return triangles; }

    ClosestPointQueryResult QueryClosestPoint(const glm::vec3& query_point) const;

    const gfx::CPUMesh* GetMesh() const { return mesh; }

private:
    struct Axis {
        u32 axis{0};  // 0: x, 1: y, 2: z
        float pos;
    };

    struct Bin {
        AABB aabb;
        u32 triangle_count{0};
    };

    SplitType split_type{SplitType::Midplane};
    const gfx::CPUMesh* mesh{nullptr};
    std::vector<Node> nodes;
    std::vector<TriangleInfo> triangles;
    static constexpr u32 max_depth = 10;

    void UpdateBounds(u32 node_idx);
    std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> GetTriangleAtIdx(
        u32 idx) const;
    const glm::vec3& GetCentroidAtIdx(u32 idx);
    void Split(u32 node_idx);
    Axis ChooseSplitPosition(const Node& node, float* cost = nullptr);
    float SurfaceAreaCost(const Node& node, int axis, float pos);

    void ClosestNode(u32 node_idx,
                     const glm::vec3& query_point,
                     ClosestPointQueryResult& result) const;
};
}  // namespace vfs
