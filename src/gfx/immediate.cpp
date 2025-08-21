#include "immediate.h"

#include "vk_util.h"

namespace gfx {
void ImmediateRunner::Init(const CoreCtx& ctx, u32 qf, VkQueue q) {
    queue_family = qf;
    queue = q;

    auto command_pool_info = vk::util::CommandPoolCreateInfo(
        queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(ctx.device, &command_pool_info, NULL, &cmd_pool));

    auto cmd_alloc_info = vk::util::CommandBufferAllocateInfo(cmd_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(ctx.device, &cmd_alloc_info, &cmd_buffer));

    auto fence_create_info = vk::util::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(ctx.device, &fence_create_info, NULL, &fence));
}

void ImmediateRunner::Submit(const CoreCtx& ctx,
                             std::function<void(VkCommandBuffer)>&& function) const {
    VK_CHECK(vkResetFences(ctx.device, 1, &fence));
    VK_CHECK(vkResetCommandBuffer(cmd_buffer, 0));

    auto cmd = cmd_buffer;
    auto cmd_begin_info =
        vk::util::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));
    auto cmd_info = vk::util::CommandBufferSubmitInfo(cmd);
    auto submit = vk::util::SubmitInfo2(&cmd_info, NULL, NULL);
    VK_CHECK(vkQueueSubmit2(queue, 1, &submit, fence));
    VK_CHECK(vkWaitForFences(ctx.device, 1, &fence, true, 200 * ONE_SEC_NS));
}

void ImmediateRunner::Clear(const CoreCtx& ctx) {
    vkDestroyCommandPool(ctx.device, cmd_pool, NULL);
    vkDestroyFence(ctx.device, fence, NULL);
}
}  // namespace gfx