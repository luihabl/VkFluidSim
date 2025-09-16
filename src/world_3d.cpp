#include "world_3d.h"

#include "gfx/common.h"
#include "platform.h"

namespace vfs {

void World3D::Init(Platform& platform) {
    auto screen_size = Platform::Info::GetScreenSize();
    camera.SetPerspective(90.0f, 0.1f, 100000.0f, (float)screen_size.x / screen_size.y);
    camera.SetPosition(gfx::axis::BACK * 20.0f);
}

void World3D::Update(Platform& platform) {}

void World3D::HandleEvent(Platform& platform, const Event& e) {}

void World3D::Clear() {}

}  // namespace vfs
