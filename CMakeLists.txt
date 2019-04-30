cmake_minimum_required(VERSION 3.12)
project(Final184 LANGUAGES CXX C)

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if (LINUX)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	find_package(X11 REQUIRED)
	find_package(XCB REQUIRED)
endif()

# Since ImGui is a submodule directly pulled from upstream, add its target here
add_library(imgui
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
)
target_include_directories(imgui INTERFACE imgui)

add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(SPIRV-Cross)
add_subdirectory(RHI)
add_subdirectory(Foreground)
add_subdirectory(App)

add_subdirectory(Shader)

