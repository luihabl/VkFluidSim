#include "imgui_setup.h"

#include "gfx/common.h"
#include "gfx/vk_util.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "platform.h"

namespace vfs {

void ImGui_Setup::Init(const gfx::Device& gfx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplSDL3_InitForVulkan(Platform::Info::GetWindow());

    auto sw_format = gfx.GetSwapchain().GetFormat();
    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = gfx.GetCoreCtx().instance,
        .PhysicalDevice = gfx.GetCoreCtx().chosen_gpu,
        .Device = gfx.GetCoreCtx().device,
        .QueueFamily = gfx.GetQueueFamily(),
        .Queue = gfx.GetQueue(),
        .DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE * 16,
        .MinImageCount = gfx::FRAME_COUNT,
        .ImageCount = gfx::FRAME_COUNT,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &sw_format,
            },
    };
    ImGui_ImplVulkan_Init(&init_info);
}  // namespace vfs

void ImGui_Setup::BeginDraw(const gfx::Device& gfx,
                            VkCommandBuffer cmd,
                            const gfx::Image& draw_image) {
    auto color_att = vk::util::RenderingAttachmentInfo(draw_image.view, NULL,
                                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    auto render_info = vk::util::RenderingInfo({draw_image.extent.width, draw_image.extent.height},
                                               &color_att, NULL);
    vkCmdBeginRendering(cmd, &render_info);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGui_Setup::EndDraw(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

void ImGui_Setup::HandleEvent(const SDL_Event& event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void ImGui_Setup::Clear(const gfx::Device& gfx) {
    ImGui_ImplVulkan_Shutdown();
}

}  // namespace vfs
