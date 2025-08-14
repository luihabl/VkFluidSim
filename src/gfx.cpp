#include "gfx.h"

#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace vfs {

void GfxDevice::Init(const GfxConfig& config) {
    vkb::InstanceBuilder builder;
    auto instance_result = builder.set_app_name(config.name)
                               .request_validation_layers(config.validation_layers)
                               .use_default_debug_messenger()
                               .require_api_version(1, 3, 0)
                               .build();

    vkb::Instance vkb_instance = instance_result.value();
    instance = vkb_instance.instance;
    debug_messenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(config.window, instance, NULL, &surface);

    auto features13 = VkPhysicalDeviceVulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    auto features12 = VkPhysicalDeviceVulkan12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    auto selector = vkb::PhysicalDeviceSelector{vkb_instance};
    auto physical_device = selector.set_minimum_version(1, 3)
                               .set_required_features_13(features13)
                               .set_required_features_12(features12)
                               .set_surface(surface)
                               .select()
                               .value();

    auto device_builder = vkb::DeviceBuilder{physical_device};
    auto vkb_device = device_builder.build().value();

    device = vkb_device.device;
    chosen_gpu = physical_device.physical_device;

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
        .physicalDevice = chosen_gpu,
        .device = device,
        .instance = instance,
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    };

    vmaCreateAllocator(&allocator_create_info, &allocator);
}

void GfxDevice::Clean() {
    vmaDestroyAllocator(allocator);
}
}  // namespace vfs