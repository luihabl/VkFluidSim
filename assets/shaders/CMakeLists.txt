find_program(SLANGC slangc)

set(SLANG_SHADERS
    simulation/lague_model.slang

    simulation/spatial_hash/scan.slang
    simulation/spatial_hash/sort.slang
    simulation/spatial_hash/update_spatial_hash.slang
    simulation/spatial_hash/spatial_offsets.slang
    simulation/spatial_hash/reorder.slang

    draw/box.slang
    draw/particles_3d.slang

)

set(SLANG_DEPENDENT_FILES
    draw/colormap.slang
    simulation/kernels/kernels_3d.slang
    simulation/spatial_hash/spatial_hash_3d.slang
)

set(SLANG_COMPILER_FLAGS
    -profile glsl_450
    -fvk-use-entrypoint-name
    -fvk-use-scalar-layout
    -emit-spirv-directly
    -target spirv
)

foreach(SLANG ${SLANG_SHADERS})
    set(SLANG ${CMAKE_CURRENT_LIST_DIR}/${SLANG})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${SLANG} NAME)
    set(SPIRV "${CMAKE_CURRENT_LIST_DIR}/compiled/${FILE_NAME}.spv")
    message(STATUS ${SLANG})
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${SLANGC} ${SLANG} ${SLANG_COMPILER_FLAGS} -o ${SPIRV}
        DEPENDS ${SLANG} ${SLANG_DEPENDENT_FILES})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(SLANG)

add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
