#pragma once

#include "gfx/mesh.h"

namespace vfs {
class MeshBVH {
public:
    struct AABB {
        glm::vec3 pos_min{std::numeric_limits<float>::max()};
        glm::vec3 pos_max{std::numeric_limits<float>::lowest()};

        void Grow(const glm::vec3& p);
        void Grow(const AABB& aabb);
        float Area() const;
    };

    struct Node {
        AABB box;
        u32 triangle_start{0};
        u32 triangle_count{0};
        u32 child_a{0};
        u32 child_b{0};

        float Cost() const;
    };

    struct TriangleInfo {
        u32 vertex_start_idx;
        glm::vec3 centroid;
    };

    enum class SplitType {
        Midplane,
        SurfaceAreaHeuristics,
        BinSplit,
    };

    void Init(gfx::CPUMesh* mesh, SplitType split = SplitType::Midplane);

    void Build();
    const std::vector<Node>& GetNodes() { return nodes; }
    const std::vector<TriangleInfo>& GetTriangleInfo() { return triangles; }

    struct ClosestPointQueryResult {
        f64 min_distance_sq{std::numeric_limits<f32>::max()};
        TriangleInfo closest_triangle;
        glm::vec3 closest_point;
    };

    ClosestPointQueryResult QueryClosestPoint(const glm::vec3& query_point);

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
    gfx::CPUMesh* mesh{nullptr};
    std::vector<Node> nodes;
    std::vector<TriangleInfo> triangles;
    static constexpr u32 max_depth = 10;

    void UpdateBounds(u32 node_idx);
    std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> GetTriangleAtIdx(u32 idx);
    const glm::vec3& GetCentroidAtIdx(u32 idx);
    void Split(u32 node_idx);
    Axis ChooseSplitPosition(const Node& node, float* cost = nullptr);
    float SurfaceAreaCost(const Node& node, int axis, float pos);

    void ClosestNode(const Node& node,
                     const glm::vec3& query_point,
                     ClosestPointQueryResult& result);
};
}  // namespace vfs
