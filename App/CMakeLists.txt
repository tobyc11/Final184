set(MODULE_NAME App)

set(CMAKE_CXX_STANDARD 17)

file(GLOB APP_SOURCES *.cpp *.h)

add_executable(${MODULE_NAME} ${APP_SOURCES})
target_link_libraries(${MODULE_NAME} PRIVATE RHI)
target_link_libraries(${MODULE_NAME} PRIVATE Foreground)

find_package(glm REQUIRED)
target_link_libraries(${MODULE_NAME} PRIVATE glm)

find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIRS}")
target_include_directories(${MODULE_NAME} PRIVATE ${ASSIMP_INCLUDE_DIRS})
target_link_directories(${MODULE_NAME} PRIVATE ${ASSIMP_LIBRARY_DIRS})
target_link_libraries(${MODULE_NAME} PRIVATE ${ASSIMP_LIBRARIES})

find_package(SDL2 REQUIRED)
if(TARGET SDL2::SDL2)
    target_link_libraries(${MODULE_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()

# We let SDL create the vulkan surface on macOS
if(APPLE)
    find_package(Vulkan REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE Vulkan::Vulkan)
endif()

if (LINUX)
        target_link_libraries(${MODULE_NAME} PRIVATE ${X11_LIBRARIES} ${XCB_LIBRARIES} X11-xcb pthread)
endif(LINUX)

add_dependencies(${MODULE_NAME} Shader)
