set(MODULE_NAME App)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

include_directories(${GLM_INCLUDE_DIRS})
link_directories(${GLM_LIBRARY_DIRS})
add_definitions(${GLM_DEFINITIONS})

find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")
include_directories(${ASSIMP_INCLUDE_DIR})

find_package(SDL2 REQUIRED)

file(GLOB APP_SOURCES *.cpp *.h)

add_executable(${MODULE_NAME} ${APP_SOURCES})
target_link_libraries(${MODULE_NAME} PRIVATE RHI)
target_link_libraries(${MODULE_NAME} PRIVATE Foreground)
target_compile_definitions(${MODULE_NAME} PRIVATE -DAPP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${MODULE_NAME} PRIVATE ${ASSIMP_LIBRARIES})

if (LINUX)
	target_link_libraries(${MODULE_NAME} PRIVATE ${X11_LIBRARIES} ${XCB_LIBRARIES} X11-xcb pthread)
endif(LINUX)

if(TARGET SDL2::SDL2)
    target_link_libraries(${MODULE_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
