# Vulkan fluid simulation

<img src="assets/figs/screenshot.png" width=50% height=50%>

This is a simple 2D SPH fluid simulation written in C++ and Vulkan. All the simulation is done through compute shaders and all the data buffers are kept in the GPU.

## Building

To build the project, you will need

- a compiler with C++20 support
- cmake
- Vulkan SDK

CMake will download almost all dependencies, except the Vulkan SDK which must be found in the path.
