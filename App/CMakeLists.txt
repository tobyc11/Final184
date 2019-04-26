set(MODULE_NAME App)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

file(GLOB APP_SOURCES *.cpp *.h)

add_executable(${MODULE_NAME} ${APP_SOURCES})
target_link_libraries(${MODULE_NAME} PRIVATE RHI)
target_link_libraries(${MODULE_NAME} PRIVATE Foreground)
target_compile_definitions(${MODULE_NAME} PRIVATE -DAPP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(TARGET SDL2::SDL2)
    target_link_libraries(${MODULE_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
