file(READ "${IMGUI_DIR}/misc/fonts/DroidSans.ttf" PKZGUI_FONT_HEX HEX)
string(REGEX REPLACE "(..)" "0x\\1," PKZGUI_FONT_BYTES "${PKZGUI_FONT_HEX}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pkzgui_font.h"
    "static const unsigned char FontData[] = {${PKZGUI_FONT_BYTES}};\n")

add_library(miniaudio STATIC audio/miniaudio_impl.c)
target_include_directories(miniaudio PUBLIC "${PROJECT_SOURCE_DIR}/external/miniaudio")
target_link_libraries(miniaudio PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
    target_link_libraries(miniaudio PRIVATE m)
endif()
set(PKZGUI_FFMPEG_EXECUTABLE "${PROJECT_SOURCE_DIR}/external/FFmpeg/ffmpeg${CMAKE_EXECUTABLE_SUFFIX}"
    CACHE FILEPATH "FFmpeg executable built alongside pkzgui")

if(APPLE OR WIN32)
    set(NATIVE_MENU_DIR "${PROJECT_SOURCE_DIR}/external/native-menu-bar")
    if(NOT EXISTS "${NATIVE_MENU_DIR}/native_menu_bar.c")
        message(FATAL_ERROR "Missing native-menu-bar: run git submodule update --init --recursive")
    endif()
    if(APPLE)
        set_source_files_properties("${NATIVE_MENU_DIR}/native_menu_bar.c"
            PROPERTIES LANGUAGE OBJC COMPILE_OPTIONS "-fno-objc-arc")
    else()
        set_source_files_properties("${NATIVE_MENU_DIR}/native_menu_bar.c"
            PROPERTIES LANGUAGE CXX)
    endif()
    add_library(native_menu_bar STATIC "${NATIVE_MENU_DIR}/native_menu_bar.c")
    target_include_directories(native_menu_bar PUBLIC "${NATIVE_MENU_DIR}")
    if(APPLE)
        target_link_libraries(native_menu_bar PRIVATE "-framework Cocoa")
    else()
        target_compile_features(native_menu_bar PRIVATE cxx_std_17)
        target_link_libraries(native_menu_bar PRIVATE user32)
    endif()
endif()

add_executable(pkzgui
    main.cpp
    ui/app_menu.cpp
    ui/workspace.cpp
    ui/workspace_actions.cpp
    package/package_session.cpp
    package/resource_selection.cpp
    texture/texture_actions.cpp
    texture/texture_preview.cpp
    audio/audio_player.cpp
    audio/audio_process.cpp
    audio/audio_sample.cpp
    platform/file_dialog.cpp
    platform/file_io.cpp)
if(APPLE)
    target_sources(pkzgui PRIVATE platform/menu_shortcut.m)
endif()
target_include_directories(pkzgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(pkzgui PRIVATE imgui pkzlib nfd Threads::Threads miniaudio)
target_compile_definitions(pkzgui PRIVATE PKZGUI_FFMPEG_PATH="${PKZGUI_FFMPEG_EXECUTABLE}")
if(EXISTS "${PKZGUI_FFMPEG_EXECUTABLE}")
    add_custom_command(TARGET pkzgui POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PKZGUI_FFMPEG_EXECUTABLE}" "$<TARGET_FILE_DIR:pkzgui>/ffmpeg${CMAKE_EXECUTABLE_SUFFIX}")
endif()
if(APPLE OR WIN32)
    target_compile_definitions(pkzgui PRIVATE PKZGUI_NATIVE_MENUS)
    target_link_libraries(pkzgui PRIVATE native_menu_bar)
endif()
target_compile_definitions(pkzgui PRIVATE GL_SILENCE_DEPRECATION)
set_target_properties(pkzgui PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(PKZGUI_BUILD_TESTS)
    add_executable(package_session_test ../tests/package_session_test.cpp package/package_session.cpp platform/file_io.cpp)
    target_include_directories(package_session_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(package_session_test PRIVATE pkzlib Threads::Threads)
    set_target_properties(package_session_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
    add_test(NAME package_session COMMAND package_session_test "${CMAKE_CURRENT_BINARY_DIR}/test-packages")
    add_executable(texture_actions_test ../tests/texture_actions_test.cpp package/package_session.cpp platform/file_io.cpp package/resource_selection.cpp texture/texture_actions.cpp)
    target_include_directories(texture_actions_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(texture_actions_test PRIVATE pkzlib Threads::Threads)
    set_target_properties(texture_actions_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
    add_test(NAME texture_actions COMMAND texture_actions_test "${CMAKE_CURRENT_BINARY_DIR}/test-textures")
    add_executable(audio_sample_test ../tests/audio_sample_test.cpp audio/audio_sample.cpp package/resource_selection.cpp audio/audio_process.cpp platform/file_io.cpp)
    target_include_directories(audio_sample_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(audio_sample_test PRIVATE pkzlib Threads::Threads)
    set_target_properties(audio_sample_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
    add_test(NAME audio_sample COMMAND audio_sample_test)
endif()
