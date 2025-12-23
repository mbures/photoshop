#include "ps/io/image_io.h"

#include <stdexcept>

#include "ps/io/png_format.h"

namespace ps::io {

void ImageIO::register_format(std::unique_ptr<ImageFormat> format) {
  if (!format) {
    throw std::invalid_argument("format must not be null");
  }
  formats_.push_back(std::move(format));
}

ps::core::ImageDocument ImageIO::load(const std::string& path) const {
  for (const auto& format : formats_) {
    if (format->can_read(path)) {
      return format->load(path);
    }
  }
  throw std::runtime_error("no registered image format can read " + path);
}

void ImageIO::save(const std::string& path, const ps::core::ImageDocument& document) const {
  for (const auto& format : formats_) {
    if (format->can_write(path, document)) {
      format->save(path, document);
      return;
    }
  }
  throw std::runtime_error("no registered image format can write " + path);
}

ImageIO create_default_image_io() {
  ImageIO io;
  io.register_format(std::make_unique<PNGFormat>());
  return io;
}

}  // namespace ps::io
