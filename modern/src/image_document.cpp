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

std::size_t ImageDocument::layer_count() const {
  return layers_.size();
}

Layer& ImageDocument::add_layer(const std::string& name) {
  layers_.push_back(std::make_unique<Layer>(size_, name));
  if (active_layer_index_ < 0) {
    active_layer_index_ = 0;
  }
  return *layers_.back();
}

Layer& ImageDocument::insert_layer(std::size_t index, const std::string& name) {
  if (index > layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }
  auto it = layers_.begin() + index;
  layers_.insert(it, std::make_unique<Layer>(size_, name));
  if (active_layer_index_ < 0) {
    active_layer_index_ = 0;
  } else if (static_cast<std::size_t>(active_layer_index_) >= index) {
    active_layer_index_++;
  }
  return *layers_[index];
}

void ImageDocument::remove_layer(std::size_t index) {
  if (index >= layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }
  layers_.erase(layers_.begin() + index);

  if (layers_.empty()) {
    active_layer_index_ = -1;
  } else if (static_cast<std::size_t>(active_layer_index_) >= layers_.size()) {
    active_layer_index_ = static_cast<int>(layers_.size()) - 1;
  }
}

const Layer& ImageDocument::layer_at(std::size_t index) const {
  if (index >= layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }
  return *layers_[index];
}

Layer& ImageDocument::layer_at(std::size_t index) {
  if (index >= layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }
  return *layers_[index];
}

const std::vector<std::unique_ptr<Layer>>& ImageDocument::layers() const {
  return layers_;
}

void ImageDocument::move_layer(std::size_t from_index, std::size_t to_index) {
  if (from_index >= layers_.size() || to_index >= layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }

  if (from_index == to_index) {
    return;
  }

  auto layer = std::move(layers_[from_index]);
  layers_.erase(layers_.begin() + from_index);

  if (to_index > from_index) {
    to_index--;
  }

  layers_.insert(layers_.begin() + to_index, std::move(layer));

  // Update active layer index
  if (active_layer_index_ == static_cast<int>(from_index)) {
    active_layer_index_ = static_cast<int>(to_index);
  } else if (from_index < to_index) {
    if (active_layer_index_ > static_cast<int>(from_index) &&
        active_layer_index_ <= static_cast<int>(to_index)) {
      active_layer_index_--;
    }
  } else {
    if (active_layer_index_ >= static_cast<int>(to_index) &&
        active_layer_index_ < static_cast<int>(from_index)) {
      active_layer_index_++;
    }
  }
}

void ImageDocument::set_active_layer(int index) {
  if (index < -1 || static_cast<std::size_t>(index) >= layers_.size()) {
    throw std::out_of_range("layer index out of range");
  }
  active_layer_index_ = index;
}

Layer* ImageDocument::active_layer() {
  if (active_layer_index_ < 0 ||
      static_cast<std::size_t>(active_layer_index_) >= layers_.size()) {
    return nullptr;
  }
  return layers_[active_layer_index_].get();
}

const Layer* ImageDocument::active_layer() const {
  if (active_layer_index_ < 0 ||
      static_cast<std::size_t>(active_layer_index_) >= layers_.size()) {
    return nullptr;
  }
  return layers_[active_layer_index_].get();
}

void ImageDocument::flatten_to_channels() {
  if (layers_.empty() || channels_.empty()) {
    return;
  }

  // Create a temporary RGBA buffer for compositing
  ImageBuffer composite(size_, PixelFormat::RGBA8);
  std::fill(composite.data(), composite.data() + composite.byte_size(), 0);

  // Composite all visible layers (will be implemented with blend modes)
  for (const auto& layer : layers_) {
    if (!layer->visible()) {
      continue;
    }

    // For now, simple alpha blending (will be enhanced with blend modes)
    const uint8_t* src = layer->buffer().data();
    uint8_t* dst = composite.data();
    const float layer_opacity = layer->opacity() / 100.0f;

    for (int i = 0; i < size_.width * size_.height; ++i) {
      const int idx = i * 4;
      const float src_alpha = (src[idx + 3] / 255.0f) * layer_opacity;
      const float dst_alpha = dst[idx + 3] / 255.0f;
      const float out_alpha = src_alpha + dst_alpha * (1.0f - src_alpha);

      if (out_alpha > 0.0f) {
        for (int c = 0; c < 3; ++c) {
          const float src_val = src[idx + c] / 255.0f;
          const float dst_val = dst[idx + c] / 255.0f;
          const float out_val = (src_val * src_alpha + dst_val * dst_alpha * (1.0f - src_alpha)) / out_alpha;
          dst[idx + c] = static_cast<uint8_t>(out_val * 255.0f);
        }
        dst[idx + 3] = static_cast<uint8_t>(out_alpha * 255.0f);
      }
    }
  }

  // Split composited RGBA into separate channels
  if (mode_ == ColorMode::RGB && channels_.size() >= 3) {
    for (int i = 0; i < size_.width * size_.height; ++i) {
      const int src_idx = i * 4;
      const int dst_idx = i * 3;  // RGB8 format stores 3 bytes per pixel

      channels_[0].buffer.data()[dst_idx] = composite.data()[src_idx];      // R
      channels_[0].buffer.data()[dst_idx + 1] = composite.data()[src_idx];  // R (replicated)
      channels_[0].buffer.data()[dst_idx + 2] = composite.data()[src_idx];  // R (replicated)

      channels_[1].buffer.data()[dst_idx] = composite.data()[src_idx + 1];     // G
      channels_[1].buffer.data()[dst_idx + 1] = composite.data()[src_idx + 1]; // G (replicated)
      channels_[1].buffer.data()[dst_idx + 2] = composite.data()[src_idx + 1]; // G (replicated)

      channels_[2].buffer.data()[dst_idx] = composite.data()[src_idx + 2];     // B
      channels_[2].buffer.data()[dst_idx + 1] = composite.data()[src_idx + 2]; // B (replicated)
      channels_[2].buffer.data()[dst_idx + 2] = composite.data()[src_idx + 2]; // B (replicated)
    }
  }
}

void ImageDocument::channels_to_layer(const std::string& name) {
  if (channels_.empty()) {
    return;
  }

  Layer& layer = add_layer(name);
  uint8_t* dst = layer.buffer().data();

  if (mode_ == ColorMode::RGB && channels_.size() >= 3) {
    for (int i = 0; i < size_.width * size_.height; ++i) {
      const int src_idx = i * 3;  // RGB8 format
      const int dst_idx = i * 4;  // RGBA8 format

      dst[dst_idx] = channels_[0].buffer.data()[src_idx];      // R
      dst[dst_idx + 1] = channels_[1].buffer.data()[src_idx + 1]; // G
      dst[dst_idx + 2] = channels_[2].buffer.data()[src_idx + 2]; // B
      dst[dst_idx + 3] = 255;  // Fully opaque
    }
  } else if (mode_ == ColorMode::Grayscale && !channels_.empty()) {
    for (int i = 0; i < size_.width * size_.height; ++i) {
      const int src_idx = i * 3;
      const int dst_idx = i * 4;
      const uint8_t gray = channels_[0].buffer.data()[src_idx];

      dst[dst_idx] = gray;
      dst[dst_idx + 1] = gray;
      dst[dst_idx + 2] = gray;
      dst[dst_idx + 3] = 255;
    }
  }
}

}  // namespace ps::core
