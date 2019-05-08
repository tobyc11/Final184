set(MODULE_NAME Pipelang)

set(CMAKE_CXX_STANDARD 17)

file(GLOB PIPELANG_SOURCES
    Private/*.h Private/*.cpp
    Public/*.h Public/*.cpp
)

add_library(${MODULE_NAME} ${PIPELANG_SOURCES})

target_compile_definitions(${MODULE_NAME} PRIVATE -DPIPELANG_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

include(GenerateExportHeader)
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(${MODULE_NAME} EXPORT_MACRO_NAME PIPELANG_API EXPORT_FILE_NAME PipelangAPI.h)

target_include_directories(${MODULE_NAME} PRIVATE Private)
target_include_directories(${MODULE_NAME} PUBLIC Public)
if(DEFAULT_COMPILE_OPTIONS)
    target_compile_options(${MODULE_NAME} ${DEFAULT_COMPILE_OPTIONS})
endif()

target_link_libraries(${MODULE_NAME} PUBLIC Foundation)
target_link_libraries(${MODULE_NAME} PUBLIC RHI)

find_package(Lua REQUIRED)
target_link_libraries(${MODULE_NAME} PRIVATE ${LUA_LIBRARIES})
target_include_directories(${MODULE_NAME} PRIVATE ${LUA_INCLUDE_DIR})

add_executable(PipelangMgr Private/Main.cpp)
target_link_libraries(PipelangMgr PRIVATE ${MODULE_NAME})
