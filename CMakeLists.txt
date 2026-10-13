cmake_minimum_required(VERSION 3.20)
project(pkzgui LANGUAGES C CXX)

foreach(dependency imgui/imgui.cpp glfw/CMakeLists.txt)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${dependency}")
        message(FATAL_ERROR "Missing external/${dependency}: run git submodule update --init --recursive")
    endif()
endforeach()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)
find_package(OpenGL REQUIRED)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
target_compile_features(imgui PUBLIC cxx_std_17)

add_subdirectory(src)
