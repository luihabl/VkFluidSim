find_program(GLSL_VALIDATOR glslangValidator)


set(SHADER_FILES
    gfx/sprite.frag
    gfx/sprite.vert

    sim/update_external_forces.comp
    sim/update_spatial_hash.comp
    sim/update_pos.comp
    sim/update_reorder.comp
    sim/update_reorder_copyback.comp

    scan/scan_block.comp
    scan/scan_combine.comp
)

set(SHADER_COMMON
    sim/common.glsl
    scan/common.glsl
    hash/spatial_hash.glsl
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
