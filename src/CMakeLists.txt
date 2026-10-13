file(READ "${IMGUI_DIR}/misc/fonts/DroidSans.ttf" PKZGUI_FONT_HEX HEX)
string(REGEX REPLACE "(..)" "0x\\1," PKZGUI_FONT_BYTES "${PKZGUI_FONT_HEX}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pkzgui_font.h"
    "static const unsigned char FontData[] = {${PKZGUI_FONT_BYTES}};\n")

add_executable(pkzgui main.cpp workspace.cpp)
target_include_directories(pkzgui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(pkzgui PRIVATE imgui)
target_compile_definitions(pkzgui PRIVATE GL_SILENCE_DEPRECATION)
set_target_properties(pkzgui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
