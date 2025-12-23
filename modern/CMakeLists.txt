cmake_minimum_required(VERSION 3.16)
project(ps_modern_core LANGUAGES CXX)

add_library(ps_modern_core
  src/image_buffer.cpp
  src/image_document.cpp
  src/command.cpp
  src/undo_stack.cpp
  src/io/image_format.cpp
  src/io/image_io.cpp
  src/io/png_format.cpp
)

find_package(PNG REQUIRED)

target_include_directories(ps_modern_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(ps_modern_core PUBLIC cxx_std_17)

target_link_libraries(ps_modern_core PUBLIC PNG::PNG)
