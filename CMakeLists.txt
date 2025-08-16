cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)

project(vfs)

include(cmake/cpm.cmake)
#add_subdirectory(resources/shaders)

# external libraries
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:g-truc/glm#1.0.1")
CPMAddPackage(
    NAME SDL
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.2.18
    OPTIONS
    "SDL_TEST_ENABLED_BY_DEFAULT Off" 
    "SDL_TEST Off"
    "SDL_SHARED Off"
    "SDL_STATIC On"
    "SDL_TEST_LIBRARY Off"
)

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    VERSION 1.92.1-docking
    DOWNLOAD_ONLY YES
)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui 
PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PRIVATE Vulkan::Vulkan SDL3::SDL3)

#CPMAddPackage("gh:tinyobjloader/tinyobjloader#release")
CPMAddPackage("gh:charles-lunarg/vk-bootstrap@1.4.323")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@3.3.0")
CPMAddPackage("gh:fmtlib/fmt#11.2.0")
#CPMAddPackage("gh:spnda/fastgltf@0.9.0")

# executable
add_executable(vfs
src/main.cpp
src/gfx/gfx.cpp
src/gfx/swapchain.cpp
src/gfx/vk_util.cpp
src/platform.cpp
src/world.cpp
src/renderer.cpp
src/pipeline.cpp
)

target_include_directories(vfs PUBLIC src)

# link stuff
target_link_libraries(vfs
PUBLIC
    Vulkan::Vulkan
    glm::glm-header-only
    SDL3::SDL3
    imgui
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    fmt::fmt
)

target_compile_definitions(vfs PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
