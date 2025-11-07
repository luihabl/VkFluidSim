cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)

project(vfs)

include(cmake/cpm.cmake)
add_subdirectory(assets/shaders)

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

add_library(vfs_gfx_platform INTERFACE)
target_link_libraries(vfs_gfx_platform INTERFACE Vulkan::Vulkan SDL3::SDL3)

target_link_libraries(imgui PUBLIC vfs_gfx_platform)

CPMAddPackage("gh:charles-lunarg/vk-bootstrap@1.4.323")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@3.3.0")
CPMAddPackage("gh:fmtlib/fmt#11.2.0")
CPMAddPackage("gh:tinyobjloader/tinyobjloader#release")

# executable
add_executable(vfs
src/main.cpp
src/platform.cpp

src/gfx/gfx.cpp
src/gfx/swapchain.cpp
src/gfx/vk_util.cpp
src/gfx/mesh.cpp
src/gfx/immediate.cpp
src/gfx/descriptor.cpp
src/gfx/common.cpp
src/gfx/transform.cpp
src/gfx/camera.cpp

src/gui/scene_renderer.cpp
src/gui/gui.cpp
src/gui/imgui_setup.cpp
src/gui/common.cpp

src/pipelines/box_pipeline.cpp
src/pipelines/mesh_pipeline.cpp
src/pipelines/particle_pipeline.cpp
src/pipelines/raymarch_pipeline.cpp

src/models/model.cpp
src/models/lague_model.cpp
src/models/wcsph_model.cpp
src/models/wcsph_with_boundary_model.cpp

src/compute/sort.cpp
src/compute/spatial_hash.cpp
src/compute/compute_pipeline.cpp

src/scenes/dam_break_scene.cpp
src/scenes/dam_break_wcsph_scene.cpp
src/scenes/model_render_scene.cpp
src/scenes/dam_break_with_objects_scene.cpp

src/util/mesh_loader.cpp
src/util/mesh_bvh.cpp
src/util/mesh_sdf.cpp
src/util/mesh_pseudonormals.cpp
src/util/geometry.cpp
src/util/gaussian_quadrature.cpp
src/util/discretization.cpp
src/util/volume_map.cpp
src/util/kernel.cpp

src/simulation.cpp
)

target_include_directories(vfs PUBLIC src)

add_dependencies(vfs shaders)

# link stuff
target_link_libraries(vfs
PUBLIC
    vfs_gfx_platform
    glm::glm-header-only
    imgui
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    fmt::fmt
    tinyobjloader
)

target_compile_definitions(vfs PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)
