#include "ps/core/layer.h"

#include <algorithm>

namespace ps::core {

Layer::Layer(Size size, const std::string& name)
    : name_(name), size_(size), buffer_(size, PixelFormat::RGBA8) {
  // Initialize buffer to transparent (all zeros)
  std::fill(buffer_.data(), buffer_.data() + buffer_.byte_size(), 0);
}

void Layer::set_opacity(int opacity) {
  opacity_ = std::clamp(opacity, 0, 100);
}

}  // namespace ps::core
