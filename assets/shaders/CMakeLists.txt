find_program(GLSL_VALIDATOR glslangValidator)


set(SHADER_FILES
    gfx/sprite.frag
    gfx/sprite.vert

    sim/update_external_forces.comp
    sim/update_spatial_hash.comp
    sim/update_pos.comp
    sim/update_reorder.comp
    sim/update_reorder_copyback.comp
    sim/update_density.comp
    sim/update_pressure_force.comp
    sim/update_viscosity.comp

    scan/scan_block.comp
    scan/scan_combine.comp

    sort/sort_clear_counts.comp
    sort/sort_calc_counts.comp
    sort/sort_scatter.comp
    sort/sort_copy_back.comp

    offsets/offsets_init.comp
    offsets/offsets_calc.comp
)

set(SHADER_COMMON
    sim/common.glsl
    sim/spatial_hash.glsl
    sim/kernels_2d.glsl
    scan/common.glsl
)


foreach(GLSL ${SHADER_FILES})
    set(GLSL ${CMAKE_CURRENT_LIST_DIR}/${GLSL})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_CURRENT_LIST_DIR}/compiled/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV})
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL} ${SHADER_COMMON})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
