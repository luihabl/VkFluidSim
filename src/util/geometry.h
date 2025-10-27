#pragma once

#include "gfx/common.h"
namespace vfs {

enum class TriangleClosestEntity { V0, V1, V2, E01, E12, E02, F };

struct TriangleDistance {
    f64 sq_distance;
    glm::dvec3 point;
    TriangleClosestEntity entity;
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
    TriangleDistance distance_info;
    f64 signed_distance;
};

SignedDistanceResult SignedDistanceToMesh(const MeshBVH& bvh,
                                          const MeshPseudonormals& pseudonormals,
                                          const glm::vec3& point);

}  // namespace vfs
