#pragma once

#include "util/discretization.h"
#include "util/mesh_sdf.h"
namespace vfs {

void GenerateVolumeMap(const MeshSDF& sdf,
                       float support_radius,
                       LinearLagrangeDiscreteGrid& volume_map);

}  // namespace vfs
