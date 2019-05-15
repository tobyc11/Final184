file(GLOB_RECURSE KERNEL_SOURCES_FRAG *.frag)
file(GLOB_RECURSE KERNEL_SOURCES_VERT *.vert)
file(GLOB_RECURSE KERNEL_SOURCES_COMP *.comp)

foreach(KERNEL ${KERNEL_SOURCES_FRAG})
    set(SPIRV "${KERNEL}.spv")
    get_filename_component(SHADER_EXTENSION ${KERNEL} EXT)
    add_custom_command(OUTPUT ${KERNEL}.spv
        COMMAND glslc -fshader-stage=fragment -I ${CMAKE_CURRENT_SOURCE_DIR} ${GLSL_COMPILE_FLAGS} ${KERNEL} -o ${SPIRV}
        DEPENDS ${KERNEL}
        COMMENT "Rebuilding ${KERNEL}.spv" )
    message(STATUS "Generating build commands for ${KERNEL}.spv")
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

foreach(KERNEL ${KERNEL_SOURCES_VERT})
    set(SPIRV "${KERNEL}.spv")
    get_filename_component(SHADER_EXTENSION ${KERNEL} EXT)
    add_custom_command(OUTPUT ${KERNEL}.spv
        COMMAND glslc -fshader-stage=vertex -I ${CMAKE_CURRENT_SOURCE_DIR} ${GLSL_COMPILE_FLAGS} ${KERNEL} -o ${SPIRV}
        DEPENDS ${KERNEL}
        COMMENT "Rebuilding ${KERNEL}.spv" )
    message(STATUS "Generating build commands for ${KERNEL}.spv")
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

foreach(KERNEL ${KERNEL_SOURCES_COMP})
    set(SPIRV "${KERNEL}.spv")
    get_filename_component(SHADER_EXTENSION ${KERNEL} EXT)
    add_custom_command(OUTPUT ${KERNEL}.spv
        COMMAND glslc -fshader-stage=compute -I ${CMAKE_CURRENT_SOURCE_DIR} ${GLSL_COMPILE_FLAGS} ${KERNEL} -o ${SPIRV}
        DEPENDS ${KERNEL}
        COMMENT "Rebuilding ${KERNEL}.spv" )
    message(STATUS "Generating build commands for ${KERNEL}.spv")
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    Shader
    DEPENDS ${SPIRV_BINARY_FILES}
)