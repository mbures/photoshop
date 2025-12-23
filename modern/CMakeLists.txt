cmake_minimum_required(VERSION 3.16)
project(ps_modern_core LANGUAGES CXX)

add_library(ps_modern_core
  src/image_buffer.cpp
  src/image_document.cpp
  src/command.cpp
  src/undo_stack.cpp
)

target_include_directories(ps_modern_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(ps_modern_core PUBLIC cxx_std_17)
