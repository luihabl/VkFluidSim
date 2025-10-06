#pragma once

#include "gfx/mesh.h"

namespace vfs {
void DrawCircleFill(gfx::CPUMesh& mesh, const glm::vec3& center, float radius, int steps);
void DrawQuad(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color);
gfx::Image CreateDrawImage(const gfx::Device& gfx, u32 w, u32 h);
gfx::Image CreateDepthImage(const gfx::Device& gfx, u32 w, u32 h);
}  // namespace vfs
