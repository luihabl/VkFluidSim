#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "gfx/mesh.h"

namespace vfs {

// This is an implementation of the pseudonormals method proposed by J. A. Baerentzen and H. Aanaes,
// “Signed Distance Computation Using the Angle Weighted Pseudonormal,” IEEE Trans. Visual. Comput.
// Graphics, vol. 11, no. 3, pp. 243–253, May 2005, doi: 10.1109/TVCG.2005.49.
class MeshPseudonormals {
public:
    void Init(const gfx::CPUMesh& mesh);
    void Build();

    const auto& TrianglePseudonormals() const { return triangle_pseudonormals; }
    const auto& EdgePseudonormals() const { return edge_pseudonormals; }
    const auto& VertexPseudonormals() const { return vertex_pseudonormals; }
    const gfx::CPUMesh* GetMesh() const { return mesh; }

private:
    const gfx::CPUMesh* mesh{nullptr};

    std::vector<glm::vec3> triangle_pseudonormals;
    std::vector<std::array<glm::vec3, 3>> edge_pseudonormals;
    std::vector<glm::vec3> vertex_pseudonormals;
};
}  // namespace vfs
