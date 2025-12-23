#include "ps/io/png_format.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#include <png.h>

namespace ps::io {
namespace {

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

png_uint_32 pixel_format_for_buffer(ps::core::PixelFormat format) {
  switch (format) {
    case ps::core::PixelFormat::Gray8:
      return PNG_FORMAT_GRAY;
    case ps::core::PixelFormat::RGB8:
      return PNG_FORMAT_RGB;
    case ps::core::PixelFormat::RGBA8:
      return PNG_FORMAT_RGBA;
    default:
      break;
  }
  throw std::runtime_error("unsupported pixel format for PNG");
}

}  // namespace

std::string PNGFormat::name() const {
  return "PNG";
}

bool PNGFormat::can_read(const std::string& path) const {
  return file_extension(path) == "png";
}

bool PNGFormat::can_write(const std::string& path,
                          const ps::core::ImageDocument& document) const {
  if (file_extension(path) != "png") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  return format == ps::core::PixelFormat::Gray8 || format == ps::core::PixelFormat::RGB8 ||
         format == ps::core::PixelFormat::RGBA8;
}

ps::core::ImageDocument PNGFormat::load(const std::string& path) const {
  png_image image;
  std::memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;

  if (!png_image_begin_read_from_file(&image, path.c_str())) {
    throw std::runtime_error(png_image_message(&image));
  }

  image.format = PNG_FORMAT_RGBA;
  std::vector<png_byte> buffer(PNG_IMAGE_SIZE(image));

  if (!png_image_finish_read(&image, nullptr, buffer.data(), 0, nullptr)) {
    const std::string message = png_image_message(&image);
    png_image_free(&image);
    throw std::runtime_error(message);
  }

  ps::core::ImageDocument document({static_cast<int>(image.width),
                                    static_cast<int>(image.height)},
                                   ps::core::ColorMode::RGB);
  auto& channel = document.add_channel("Composite", ps::core::PixelFormat::RGBA8);
  std::memcpy(channel.buffer.data(), buffer.data(), buffer.size());
  return document;
}

void PNGFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  png_image image;
  std::memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;
  image.width = static_cast<png_uint_32>(buffer.size().width);
  image.height = static_cast<png_uint_32>(buffer.size().height);
  image.format = pixel_format_for_buffer(buffer.format());

  if (!png_image_write_to_file(&image, path.c_str(), 0, buffer.data(), 0, nullptr)) {
    throw std::runtime_error(png_image_message(&image));
  }
}

}  // namespace ps::io
