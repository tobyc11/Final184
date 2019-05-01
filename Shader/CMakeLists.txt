file(GLOB KERNEL_SOURCES *.frag *.vert)

foreach(KERNEL ${KERNEL_SOURCES})
    set(SPIRV "${KERNEL}.spv")
    add_custom_command(OUTPUT ${KERNEL}.spv
        COMMAND glslc --target-env=vulkan ${GLSL_COMPILE_FLAGS} ${KERNEL} -o ${SPIRV}
        DEPENDS ${KERNEL}
        COMMENT "Rebuilding ${KERNEL}.spv" )
    message(STATUS "Generating build commands for ${KERNEL}.spv")
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    Shader
    DEPENDS ${SPIRV_BINARY_FILES}
)