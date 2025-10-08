#pragma once

#include "gfx/mesh.h"
namespace vfs {
class MeshBVH {
public:
    struct Node {
        gfx::BoundingBox bbox;
        u32 triangle_start{0};
        u32 triangle_count{0};
        u32 children_start{0};
    };

    void Init(gfx::CPUMesh* mesh);
    void Build();
    const std::vector<Node>& GetNodes() { return nodes; }

private:
    gfx::CPUMesh* mesh{nullptr};
    std::vector<Node> nodes;
    std::vector<u32> triangle_start_idx;

    gfx::BoundingBox GetBoundingBox(u32 start, u32 count);
    void Split(u32 root, u32 depth = 0);

    struct Axis {
        u32 axis{0};  // 0: x, 1: y, 2: z
        float pos;
    };
    Axis ChooseSplitPosition(u32 parent);
    void GrowBoundingBoxToInclude(gfx::BoundingBox& bbox, u32 triangle_idx);

    static constexpr u32 max_depth = 10;
};
}  // namespace vfs
