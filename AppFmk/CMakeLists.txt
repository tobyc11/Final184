set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(CURR_DIR_SRCS EditorWindow.cpp EditorWindow.h EditorWindow.ui EditorView.cpp EditorView.h)

add_executable(TypicalApp WIN32 ${CURR_DIR_SRCS})
target_link_libraries(TypicalApp Qt5::Widgets)
target_link_libraries(TypicalApp Foundation)
target_link_libraries(TypicalApp RHI)
