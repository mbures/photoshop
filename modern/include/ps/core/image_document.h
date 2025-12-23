#pragma once

#include <string>
#include <vector>

#include "ps/core/image_buffer.h"

namespace ps::core {

enum class ColorMode {
  Grayscale,
  RGB,
  CMYK
};

struct ImageChannel {
  std::string name;
  ImageBuffer buffer;
};

class ImageDocument {
 public:
  explicit ImageDocument(Size size, ColorMode mode);

  Size size() const;
  ColorMode mode() const;

  const std::vector<ImageChannel>& channels() const;
  std::vector<ImageChannel>& channels();

  ImageChannel& add_channel(const std::string& name, PixelFormat format);
  ImageChannel& channel_at(std::size_t index);
  const ImageChannel& channel_at(std::size_t index) const;

 private:
  Size size_{};
  ColorMode mode_ = ColorMode::RGB;
  std::vector<ImageChannel> channels_{};
};

}  // namespace ps::core
