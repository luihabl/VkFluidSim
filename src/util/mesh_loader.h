#pragma once

#include <string>

#include "gfx/mesh.h"
namespace vfs {
std::vector<gfx::CPUMesh> LoadObjMesh(const std::string& path);
}  // namespace vfs
