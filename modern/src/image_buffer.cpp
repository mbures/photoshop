#include "ps/core/image_buffer.h"

namespace ps::core {

std::size_t bytes_per_pixel(PixelFormat format) {
  switch (format) {
    case PixelFormat::Gray8:
      return 1;
    case PixelFormat::RGB8:
      return 3;
    case PixelFormat::RGBA8:
      return 4;
    case PixelFormat::CMYK8:
      return 4;
  }
  return 0;
}

ImageBuffer::ImageBuffer(Size size, PixelFormat format) {
  resize(size, format);
}

Size ImageBuffer::size() const {
  return size_;
}

PixelFormat ImageBuffer::format() const {
  return format_;
}

std::uint8_t* ImageBuffer::data() {
  return pixels_.data();
}

const std::uint8_t* ImageBuffer::data() const {
  return pixels_.data();
}

std::size_t ImageBuffer::byte_size() const {
  return pixels_.size();
}

void ImageBuffer::resize(Size size, PixelFormat format) {
  size_ = size;
  format_ = format;

  const auto pixel_count = static_cast<std::size_t>(size.width) *
                           static_cast<std::size_t>(size.height);
  pixels_.assign(pixel_count * bytes_per_pixel(format_), 0);
}

}  // namespace ps::core
