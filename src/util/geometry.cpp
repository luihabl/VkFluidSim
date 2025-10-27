#include "geometry.h"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include "mesh_bvh.h"
#include "mesh_pseudonormals.h"

namespace vfs {
TriangleDistance DistancePointToTriangle(const glm::dvec3& p,
                                         const glm::dvec3& v0,
                                         const glm::dvec3& v1,
                                         const glm::dvec3& v2) {
    TriangleDistance result;

    auto diff = v0 - p;
    auto edge0 = v1 - v0;
    auto edge1 = v2 - v0;
    auto a00 = glm::dot(edge0, edge0);
    auto a01 = glm::dot(edge0, edge1);
    auto a11 = glm::dot(edge1, edge1);
    auto b0 = glm::dot(diff, edge0);
    auto b1 = glm::dot(diff, edge1);
    auto c = glm::dot(diff, diff);
    auto det = std::abs(a00 * a11 - a01 * a01);
    auto s = a01 * b1 - a11 * b0;
    auto t = a01 * b0 - a00 * b1;

    auto d2 = -1.0;

    if (s + t <= det) {
        if (s < 0) {
            if (t < 0)  // region 4
            {
                if (b0 < 0) {
                    t = 0;
                    if (-b0 >= a00) {
                        result.entity = TriangleClosestEntity::V1;
                        s = 1;
                        d2 = a00 + (2) * b0 + c;
                    } else {
                        result.entity = TriangleClosestEntity::E01;
                        s = -b0 / a00;
                        d2 = b0 * s + c;
                    }
                } else {
                    s = 0;
                    if (b1 >= 0) {
                        result.entity = TriangleClosestEntity::V0;
                        t = 0;
                        d2 = c;
                    } else if (-b1 >= a11) {
                        result.entity = TriangleClosestEntity::V2;
                        t = 1;
                        d2 = a11 + (2) * b1 + c;
                    } else {
                        result.entity = TriangleClosestEntity::E02;
                        t = -b1 / a11;
                        d2 = b1 * t + c;
                    }
                }
            } else  // region 3
            {
                s = 0;
                if (b1 >= 0) {
                    result.entity = TriangleClosestEntity::V0;
                    t = 0;
                    d2 = c;
                } else if (-b1 >= a11) {
                    result.entity = TriangleClosestEntity::V2;
                    t = 1;
                    d2 = a11 + (2) * b1 + c;
                } else {
                    result.entity = TriangleClosestEntity::E02;
                    t = -b1 / a11;
                    d2 = b1 * t + c;
                }
            }
        } else if (t < 0)  // region 5
        {
            t = 0;
            if (b0 >= 0) {
                result.entity = TriangleClosestEntity::V0;
                s = 0;
                d2 = c;
            } else if (-b0 >= a00) {
                result.entity = TriangleClosestEntity::V1;
                s = 1;
                d2 = a00 + (2) * b0 + c;
            } else {
                result.entity = TriangleClosestEntity::E01;
                s = -b0 / a00;
                d2 = b0 * s + c;
            }
        } else  // region 0
        {
            result.entity = TriangleClosestEntity::F;
            // minimum at interior point
            double inv_det = (1) / det;
            s *= inv_det;
            t *= inv_det;
            d2 = s * (a00 * s + a01 * t + (2) * b0) + t * (a01 * s + a11 * t + (2) * b1) + c;
        }
    } else {
        double tmp0, tmp1, numer, denom;

        if (s < 0)  // region 2
        {
            tmp0 = a01 + b0;
            tmp1 = a11 + b1;
            if (tmp1 > tmp0) {
                numer = tmp1 - tmp0;
                denom = a00 - (2) * a01 + a11;
                if (numer >= denom) {
                    result.entity = TriangleClosestEntity::V1;
                    s = 1;
                    t = 0;
                    d2 = a00 + (2) * b0 + c;
                } else {
                    result.entity = TriangleClosestEntity::E12;
                    s = numer / denom;
                    t = 1 - s;
                    d2 =
                        s * (a00 * s + a01 * t + (2) * b0) + t * (a01 * s + a11 * t + (2) * b1) + c;
                }
            } else {
                s = 0;
                if (tmp1 <= 0) {
                    result.entity = TriangleClosestEntity::V2;
                    t = 1;
                    d2 = a11 + (2) * b1 + c;
                } else if (b1 >= 0) {
                    result.entity = TriangleClosestEntity::V0;
                    t = 0;
                    d2 = c;
                } else {
                    result.entity = TriangleClosestEntity::E02;
                    t = -b1 / a11;
                    d2 = b1 * t + c;
                }
            }
        } else if (t < 0)  // region 6
        {
            tmp0 = a01 + b1;
            tmp1 = a00 + b0;
            if (tmp1 > tmp0) {
                numer = tmp1 - tmp0;
                denom = a00 - (2) * a01 + a11;
                if (numer >= denom) {
                    result.entity = TriangleClosestEntity::V2;
                    t = 1;
                    s = 0;
                    d2 = a11 + (2) * b1 + c;
                } else {
                    result.entity = TriangleClosestEntity::E12;
                    t = numer / denom;
                    s = 1 - t;
                    d2 =
                        s * (a00 * s + a01 * t + (2) * b0) + t * (a01 * s + a11 * t + (2) * b1) + c;
                }
            } else {
                t = 0;
                if (tmp1 <= 0) {
                    result.entity = TriangleClosestEntity::V1;
                    s = 1;
                    d2 = a00 + (2) * b0 + c;
                } else if (b0 >= 0) {
                    result.entity = TriangleClosestEntity::V0;
                    s = 0;
                    d2 = c;
                } else {
                    result.entity = TriangleClosestEntity::E01;
                    s = -b0 / a00;
                    d2 = b0 * s + c;
                }
            }
        } else  // region 1
        {
            numer = a11 + b1 - a01 - b0;
            if (numer <= 0) {
                result.entity = TriangleClosestEntity::V2;
                s = 0;
                t = 1;
                d2 = a11 + (2) * b1 + c;
            } else {
                denom = a00 - (2) * a01 + a11;
                if (numer >= denom) {
                    result.entity = TriangleClosestEntity::V1;
                    s = 1;
                    t = 0;
                    d2 = a00 + (2) * b0 + c;
                } else {
                    result.entity = TriangleClosestEntity::E12;
                    s = numer / denom;
                    t = 1 - s;
                    d2 =
                        s * (a00 * s + a01 * t + (2) * b0) + t * (a01 * s + a11 * t + (2) * b1) + c;
                }
            }
        }
    }

    // Account for numerical round-off error.
    if (d2 < 0) {
        d2 = 0;
    }

    result.point = v0 + s * edge0 + t * edge1;
    result.sq_distance = d2;
    return result;
}

AABBDistance DistancePointToAABB(const glm::dvec3& p,
                                 const glm::dvec3& min_pos,
                                 const glm::dvec3& max_pos) {
    AABBDistance result;

    auto half_size = (max_pos - min_pos) * 0.5;
    auto center = max_pos - half_size;
    auto pc = p - center;

    result.point = pc;
    result.sq_distance = 0;

    for (u32 i = 0; i < 3; i++) {
        if (pc[i] < -half_size[i]) {
            auto delta = result.point[i] + half_size[i];
            result.sq_distance += delta * delta;
            result.point[i] = -half_size[i];
        } else if (pc[i] > half_size[i]) {
            auto delta = result.point[i] - half_size[i];
            result.sq_distance += delta * delta;
            result.point[i] = half_size[i];
        }
    }

    result.point += center;

    return result;
}

SignedDistanceResult SignedDistanceToMesh(const MeshBVH& bvh,
                                          const MeshPseudonormals& pseudonormals,
                                          const glm::vec3& point) {
    auto query_result = bvh.QueryClosestPoint(point);

    const auto* mesh = pseudonormals.GetMesh();

    glm::vec3 pseudonormal;

    switch (query_result.closest_entity) {
        case TriangleClosestEntity::V0: {
            const auto& vertex_pn = pseudonormals.VertexPseudonormals();
            auto vertex_id = mesh->indices[query_result.closest_triangle.vertex_start_idx + 0];
            pseudonormal = vertex_pn[vertex_id];
            break;
        }
        case TriangleClosestEntity::V1: {
            const auto& vertex_pn = pseudonormals.VertexPseudonormals();
            auto vertex_id = mesh->indices[query_result.closest_triangle.vertex_start_idx + 1];
            pseudonormal = vertex_pn[vertex_id];
            break;
        }
        case TriangleClosestEntity::V2: {
            const auto& vertex_pn = pseudonormals.VertexPseudonormals();
            auto vertex_id = mesh->indices[query_result.closest_triangle.vertex_start_idx + 2];
            pseudonormal = vertex_pn[vertex_id];
            break;
        }
        case TriangleClosestEntity::E01: {
            const auto& edge_pn = pseudonormals.EdgePseudonormals();
            pseudonormal = edge_pn[query_result.closest_triangle.vertex_start_idx / 3][0];
            break;
        }
        case TriangleClosestEntity::E12: {
            const auto& edge_pn = pseudonormals.EdgePseudonormals();
            pseudonormal = edge_pn[query_result.closest_triangle.vertex_start_idx / 3][1];
            break;
        }
        case TriangleClosestEntity::E02: {
            const auto& edge_pn = pseudonormals.EdgePseudonormals();
            pseudonormal = edge_pn[query_result.closest_triangle.vertex_start_idx / 3][2];
            break;
        }
        case TriangleClosestEntity::F: {
            const auto& triangle_pn = pseudonormals.TrianglePseudonormals();
            pseudonormal = triangle_pn[query_result.closest_triangle.vertex_start_idx / 3];
            break;
        }
    }

    auto u = point - query_result.closest_point;
    auto sign = glm::dot(u, pseudonormal) >= 0 ? 1.0 : -1.0;

    auto result = SignedDistanceResult{};
    result.distance_info = {
        .sq_distance = query_result.min_distance_sq,
        .entity = query_result.closest_entity,
        .point = query_result.closest_point,
    };

    result.signed_distance = sign * std::sqrt(query_result.min_distance_sq);

    return result;
}

}  // namespace vfs
