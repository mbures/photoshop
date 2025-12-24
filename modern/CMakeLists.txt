cmake_minimum_required(VERSION 3.16)
project(ps_modern_core LANGUAGES CXX)

include(FetchContent)

add_library(ps_modern_core
  src/image_buffer.cpp
  src/image_document.cpp
  src/command.cpp
  src/undo_stack.cpp
  src/selection_mask.cpp
  src/selection_command.cpp
  src/tools/tool.cpp
  src/tools/brush_tool.cpp
  src/tools/selection_tools.cpp
  src/tools/tool_manager.cpp
  src/rendering/viewport.cpp
  src/rendering/canvas.cpp
  src/io/image_format.cpp
  src/io/image_io.cpp
  src/io/png_format.cpp
)

find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.9.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(ps_imgui
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
)

target_include_directories(ps_imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_compile_features(ps_imgui PUBLIC cxx_std_17)
target_link_libraries(ps_imgui PUBLIC SDL2::SDL2 OpenGL::GL)

target_include_directories(ps_modern_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(ps_modern_core PUBLIC cxx_std_17)

target_link_libraries(ps_modern_core PUBLIC PNG::PNG)

add_executable(ps_modern_app
  src/app/main.cpp
)

target_compile_features(ps_modern_app PUBLIC cxx_std_17)
target_link_libraries(ps_modern_app
  PRIVATE
    ps_modern_core
    ps_imgui
    SDL2::SDL2
    OpenGL::GL
)
