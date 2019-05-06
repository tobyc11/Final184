set(MODULE_NAME Foreground)

set(CMAKE_CXX_STANDARD 17)

file(GLOB RENDER_SOURCES
	*.h *.cpp
    Material/*.h Material/*.cpp
    Renderer/*.h Renderer/*.cpp
    Resources/*.h Resources/*.cpp
    SceneGraph/*.h SceneGraph/*.cpp
    Shader/*.h Shader/*.cpp
    Shape/*.h Shape/*.cpp
    Components/*.h Components/*.cpp
)

add_library(${MODULE_NAME} ${RENDER_SOURCES})

target_compile_definitions(${MODULE_NAME} PRIVATE -DAPP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../")

find_package(glm REQUIRED)
target_link_libraries(${MODULE_NAME} PRIVATE glm)

include(GenerateExportHeader)
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(${MODULE_NAME} EXPORT_MACRO_NAME FOREGROUND_API EXPORT_FILE_NAME ForegroundAPI.h)

target_link_libraries(BackendPriv INTERFACE spirv-cross-glsl)

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(DEFAULT_COMPILE_OPTIONS)
    target_compile_options(${MODULE_NAME} ${DEFAULT_COMPILE_OPTIONS})
endif()

target_link_libraries(${MODULE_NAME} PUBLIC RHI)
target_link_libraries(${MODULE_NAME} PUBLIC Math)
target_link_libraries(${MODULE_NAME} PUBLIC imgui)
