#pragma once

#include <string>

#include "scenes/scene.h"

namespace vfs {
std::unique_ptr<SceneBase> LoadScene(gfx::Device& gfx, const std::string& config_path);
}  // namespace vfs
