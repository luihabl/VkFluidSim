#pragma once

#include "gfx/gfx.h"
namespace vfs {
class UI {
public:
    void Init(const gfx::Device& gfx);
    void BeginDraw(const gfx::Device& gfx, VkCommandBuffer cmd, const gfx::Image& img);
    void EndDraw(VkCommandBuffer cmd);
    void Clear(const gfx::Device& gfx);
    void HandleEvent(const SDL_Event& event);

private:
};
}  // namespace vfs
