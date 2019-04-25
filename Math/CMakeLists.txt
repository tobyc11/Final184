set(MODULE_NAME Math)

set(CMAKE_CXX_STANDARD 11)

file(GLOB URHO3DMATH_PRIVATE_SOURCES *.cpp)
file(GLOB URHO3DMATH_PUBLIC_SOURCES *.h)

add_library(${MODULE_NAME} ${URHO3DMATH_PRIVATE_SOURCES} ${URHO3DMATH_PUBLIC_SOURCES})

#Generate export header
include(GenerateExportHeader)
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(${MODULE_NAME} EXPORT_FILE_NAME MathAPI.h)

target_include_directories(${MODULE_NAME} PUBLIC .)
target_compile_definitions(${MODULE_NAME} PUBLIC URHO3D_SSE)

if(MSVC)
	target_compile_definitions(${MODULE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(DEFAULT_COMPILE_OPTIONS)
    target_compile_options(${MODULE_NAME} ${DEFAULT_COMPILE_OPTIONS})
endif()
