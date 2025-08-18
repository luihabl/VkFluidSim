#include "gfx/common.h"
#pragma

namespace gfx {
class ImmediateRunner {
public:
    void Init(const CoreCtx& ctx, u32 queue_family, VkQueue queue);
    void Submit(const CoreCtx& ctx, std::function<void(VkCommandBuffer)>&& function) const;
    void Clean(const CoreCtx& ctx);

private:
    VkFence fence;
    VkCommandBuffer cmd_buffer;
    VkCommandPool cmd_pool;

    u32 queue_family;
    VkQueue queue;
};
}  // namespace gfx