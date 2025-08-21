#include "gfx.h"

#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "SDL3/SDL_video.h"
#include "gfx/common.h"
#include "vk_util.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace gfx {

Image Device::CreateImage(VkExtent3D size,
                          VkFormat format,
                          VkImageUsageFlags usage,
                          bool mip) const {
    auto img = Image{
        .format = format,
        .extent = size,
    };

    auto img_info = vk::util::ImageCreateInfo(format, usage, size);

    if (mip) {
        // TODO: check this
        img_info.mipLevels = (uint32_t)std::floor(std::log2(std::max(size.width, size.height))) + 1;
    }

    auto alloc_info = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_CHECK(vmaCreateImage(allocator, &img_info, &alloc_info, &img.image, &img.allocation, NULL));

    auto aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto view_info = vk::util::ImageViewCreateInfo(format, img.image, aspect_flag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;
    VK_CHECK(vkCreateImageView(core.device, &view_info, NULL, &img.view));

    return img;
}

void Device::DestroyImage(Image& img) const {
    if (img.image != VK_NULL_HANDLE) {
        vkDestroyImageView(core.device, img.view, NULL);
        vmaDestroyImage(allocator, img.image, img.allocation);
        img.image = VK_NULL_HANDLE;
    }
}

Buffer Device::CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage) const {
    auto buffer_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = size,
        .usage = usage,
    };

    auto vma_alloc_info = VmaAllocationCreateInfo{
        .usage = mem_usage,
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
    };

    Buffer buffer{.size = (u32)size};
    VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_alloc_info, &buffer.buffer,
                             &buffer.alloc, &buffer.info));
    return buffer;
}

void Device::DestroyBuffer(Buffer& buf) const {
    vmaDestroyBuffer(allocator, buf.buffer, buf.alloc);
}

u32 Device::CurrentFrameIndex() {
    return frame_number % gfx::FRAME_COUNT;
}

Device::FrameData& Device::CurrentFrame() {
    return frames[CurrentFrameIndex()];
}

VkCommandBuffer Device::BeginFrame() {
    swapchain.BeginFrame(core, CurrentFrameIndex());

    auto& frame = CurrentFrame();
    auto cmd = frame.cmd_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    auto cmd_begin_info =
        vk::util::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    return cmd;
}

VkExtent2D Device::GetSwapchainExtent() {
    return swapchain.GetExtent();
};

void Device::EndFrame(VkCommandBuffer cmd, const Image& draw_img) {
    u32 swapchain_img_index;
    auto swapchain_img =
        swapchain.AcquireNextImage(core, CurrentFrameIndex(), &swapchain_img_index);
    if (swapchain_img == VK_NULL_HANDLE) {
        return;
    }

    swapchain.ResetFences(core, CurrentFrameIndex());

    // clear swapchain image
    {
        auto clear_range = vk::util::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vk::util::TransitionImage(cmd, swapchain_img, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_GENERAL);
        const auto clear_color_val = VkClearColorValue{
            swapchain_img_clear_color.r,
            swapchain_img_clear_color.g,
            swapchain_img_clear_color.b,
            swapchain_img_clear_color.a,
        };

        vkCmdClearColorImage(cmd, swapchain_img, VK_IMAGE_LAYOUT_GENERAL, &clear_color_val, 1,
                             &clear_range);
    }

    // copy draw image to swapchain image
    {
        vk::util::TransitionImage(cmd, swapchain_img, VK_IMAGE_LAYOUT_GENERAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk::util::TransitionImage(cmd, draw_img.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vk::util::CopyImage(
            cmd, draw_img.image, swapchain_img,
            VkExtent2D{.width = draw_img.extent.width, .height = draw_img.extent.height},
            swapchain.GetExtent());
    }

    // present
    {
        vk::util::TransitionImage(cmd, swapchain_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        VK_CHECK(vkEndCommandBuffer(cmd));
        swapchain.SubmitAndPresent(cmd, graphics_queue, CurrentFrameIndex(), swapchain_img_index);
    }

    frame_number++;
}

void Device::InitCommandBuffers() {
    auto command_pool_info = vk::util::CommandPoolCreateInfo(
        graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        auto& frame = frames[i];
        VK_CHECK(vkCreateCommandPool(core.device, &command_pool_info, NULL, &frame.cmd_pool));
        auto cmd_buf_alloc_info = vk::util::CommandBufferAllocateInfo(frame.cmd_pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(core.device, &cmd_buf_alloc_info, &frame.cmd_buffer));
    }
}

void Device::Init(const Config& config) {
    vkb::InstanceBuilder builder;
    auto instance_result = builder.set_app_name(config.name)
                               .request_validation_layers(config.validation_layers)
                               .use_default_debug_messenger()
                               .require_api_version(1, 3, 0)
                               .build();

    vkb::Instance vkb_instance = instance_result.value();
    core.instance = vkb_instance.instance;
    core.debug_messenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(config.window, core.instance, NULL, &core.surface);

    auto features13 = VkPhysicalDeviceVulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    auto features12 = VkPhysicalDeviceVulkan12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.scalarBlockLayout = true;

    auto selector = vkb::PhysicalDeviceSelector{vkb_instance};
    auto physical_device = selector.set_minimum_version(1, 3)
                               .set_required_features_13(features13)
                               .set_required_features_12(features12)
                               .set_surface(core.surface)
                               .select()
                               .value();

    auto device_builder = vkb::DeviceBuilder{physical_device};
    auto vkb_device = device_builder.build().value();

    core.device = vkb_device.device;
    core.chosen_gpu = physical_device.physical_device;

    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(physical_device, &device_props);

    uint32_t driver_version = device_props.driverVersion;
    uint32_t version_major = VK_VERSION_MAJOR(driver_version);
    uint32_t version_minor = VK_VERSION_MINOR(driver_version);
    uint32_t version_patch = VK_VERSION_PATCH(driver_version);

    printf("%s %u.%u.%u\n", device_props.deviceName, version_major, version_minor, version_patch);

    uint32_t vulkan_version;
    vkEnumerateInstanceVersion(&vulkan_version);
    uint32_t vulkan_version_major = VK_VERSION_MAJOR(vulkan_version);
    uint32_t vulkan_version_minor = VK_VERSION_MINOR(vulkan_version);
    uint32_t vulkan_version_patch = VK_VERSION_PATCH(vulkan_version);
    uint32_t vulkan_used_version_major = VK_VERSION_MAJOR(vkb_instance.api_version);
    uint32_t vulkan_used_version_minor = VK_VERSION_MINOR(vkb_instance.api_version);
    uint32_t vulkan_used_version_patch = VK_VERSION_PATCH(vkb_instance.api_version);

    printf("Vulkan %u.%u.%u [%u.%u.%u]\n", vulkan_used_version_major, vulkan_used_version_minor,
           vulkan_used_version_patch, vulkan_version_major, vulkan_version_minor,
           vulkan_version_patch);

    graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    auto allocator_create_info = VmaAllocatorCreateInfo{
        .physicalDevice = core.chosen_gpu,
        .device = core.device,
        .instance = core.instance,
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    };

    vmaCreateAllocator(&allocator_create_info, &allocator);

    int w, h;
    SDL_GetWindowSize(config.window, &w, &h);

    swapchain.Create(core, w, h);
    swapchain.InitSyncStructs(core);
    swapchain_img_clear_color = glm::vec4(0.0f);

    InitCommandBuffers();

    immediate_runner.Init(core, graphics_queue_family, graphics_queue);
}

void Device::ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function) const {
    immediate_runner.Submit(core, std::move(function));
}

void Device::Clear() {
    vkDeviceWaitIdle(core.device);

    immediate_runner.Clear(core);

    swapchain.Destroy(core);

    for (auto& frame : frames) {
        vkDestroyCommandPool(core.device, frame.cmd_pool, NULL);
    }

    vmaDestroyAllocator(allocator);
}
}  // namespace gfx