#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ps::core {

struct Size {
  int width = 0;
  int height = 0;
};

enum class PixelFormat {
  Gray8,
  RGB8,
  RGBA8,
  CMYK8
};

std::size_t bytes_per_pixel(PixelFormat format);

class ImageBuffer {
 public:
  ImageBuffer() = default;
  ImageBuffer(Size size, PixelFormat format);

  Size size() const;
  PixelFormat format() const;

  std::uint8_t* data();
  const std::uint8_t* data() const;
  std::size_t byte_size() const;

  void resize(Size size, PixelFormat format);

 private:
  Size size_{};
  PixelFormat format_ = PixelFormat::RGB8;
  std::vector<std::uint8_t> pixels_{};
};

}  // namespace ps::core
