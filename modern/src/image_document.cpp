#include "ps/core/image_document.h"

#include <stdexcept>

namespace ps::core {

ImageDocument::ImageDocument(Size size, ColorMode mode)
    : size_(size), mode_(mode), selection_(size) {}

Size ImageDocument::size() const {
  return size_;
}

ColorMode ImageDocument::mode() const {
  return mode_;
}

const std::vector<ImageChannel>& ImageDocument::channels() const {
  return channels_;
}

std::vector<ImageChannel>& ImageDocument::channels() {
  return channels_;
}

ImageChannel& ImageDocument::add_channel(const std::string& name, PixelFormat format) {
  channels_.push_back({name, ImageBuffer(size_, format)});
  return channels_.back();
}

ImageChannel& ImageDocument::channel_at(std::size_t index) {
  if (index >= channels_.size()) {
    throw std::out_of_range("channel index out of range");
  }
  return channels_[index];
}

const ImageChannel& ImageDocument::channel_at(std::size_t index) const {
  if (index >= channels_.size()) {
    throw std::out_of_range("channel index out of range");
  }
  return channels_[index];
}

const SelectionMask& ImageDocument::selection() const {
  return selection_;
}

SelectionMask& ImageDocument::selection() {
  return selection_;
}

}  // namespace ps::core
