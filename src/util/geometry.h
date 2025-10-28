#pragma once

#include <limits>

#include "gfx/common.h"
namespace vfs {

struct AABB {
    glm::vec3 pos_min{std::numeric_limits<float>::max()};
    glm::vec3 pos_max{std::numeric_limits<float>::lowest()};

    void Grow(const glm::vec3& p);
    void Grow(const AABB& aabb);
    float Area() const;
};

enum class TriangleClosestEntity { V0, V1, V2, E01, E12, E02, F };

struct TriangleDistance {
    f64 sq_distance;
    glm::dvec3 point;
    TriangleClosestEntity entity;
};

struct TriangleInfo {
    u32 vertex_start_idx;
    glm::vec3 centroid;
};

struct ClosestPointQueryResult {
    f64 min_distance_sq{std::numeric_limits<f32>::max()};
    TriangleInfo closest_triangle;
    TriangleClosestEntity closest_entity;
    glm::vec3 closest_point;
};

TriangleDistance DistancePointToTriangle(const glm::dvec3& p,
                                         const glm::dvec3& v0,
                                         const glm::dvec3& v1,
                                         const glm::dvec3& v2);

struct AABBDistance {
    f64 sq_distance;
    glm::dvec3 point;
};

AABBDistance DistancePointToAABB(const glm::dvec3& p,
                                 const glm::dvec3& min_pos,
                                 const glm::dvec3& max_pos);

class MeshBVH;
class MeshPseudonormals;

struct SignedDistanceResult {
    ClosestPointQueryResult query_result;
    f64 signed_distance;
};

SignedDistanceResult SignedDistanceToMesh(const MeshBVH& bvh,
                                          const MeshPseudonormals& pseudonormals,
                                          const glm::vec3& point);

}  // namespace vfs
