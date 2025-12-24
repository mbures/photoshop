#include "ps/core/channel_operations.h"

#include <algorithm>

namespace ps::core {

std::vector<std::unique_ptr<Layer>> split_layer_to_channels(
    const Layer& source, bool include_alpha) {
  std::vector<std::unique_ptr<Layer>> result;

  const Size size = source.size();
  const uint8_t* src = source.buffer().data();

  // Create Red channel layer
  auto red_layer = std::make_unique<Layer>(size, "Red");
  uint8_t* red_dst = red_layer->buffer().data();

  // Create Green channel layer
  auto green_layer = std::make_unique<Layer>(size, "Green");
  uint8_t* green_dst = green_layer->buffer().data();

  // Create Blue channel layer
  auto blue_layer = std::make_unique<Layer>(size, "Blue");
  uint8_t* blue_dst = blue_layer->buffer().data();

  // Optionally create Alpha channel layer
  std::unique_ptr<Layer> alpha_layer;
  uint8_t* alpha_dst = nullptr;

  if (include_alpha) {
    alpha_layer = std::make_unique<Layer>(size, "Alpha");
    alpha_dst = alpha_layer->buffer().data();
  }

  // Split channels
  const int pixel_count = size.width * size.height;
  for (int i = 0; i < pixel_count; ++i) {
    const int src_idx = i * 4;
    const int dst_idx = i * 4;

    const uint8_t r = src[src_idx];
    const uint8_t g = src[src_idx + 1];
    const uint8_t b = src[src_idx + 2];
    const uint8_t a = src[src_idx + 3];

    // Red channel (as grayscale)
    red_dst[dst_idx] = r;
    red_dst[dst_idx + 1] = r;
    red_dst[dst_idx + 2] = r;
    red_dst[dst_idx + 3] = 255;

    // Green channel (as grayscale)
    green_dst[dst_idx] = g;
    green_dst[dst_idx + 1] = g;
    green_dst[dst_idx + 2] = g;
    green_dst[dst_idx + 3] = 255;

    // Blue channel (as grayscale)
    blue_dst[dst_idx] = b;
    blue_dst[dst_idx + 1] = b;
    blue_dst[dst_idx + 2] = b;
    blue_dst[dst_idx + 3] = 255;

    // Alpha channel (as grayscale)
    if (alpha_dst) {
      alpha_dst[dst_idx] = a;
      alpha_dst[dst_idx + 1] = a;
      alpha_dst[dst_idx + 2] = a;
      alpha_dst[dst_idx + 3] = 255;
    }
  }

  result.push_back(std::move(red_layer));
  result.push_back(std::move(green_layer));
  result.push_back(std::move(blue_layer));

  if (alpha_layer) {
    result.push_back(std::move(alpha_layer));
  }

  return result;
}

std::unique_ptr<Layer> merge_channels_to_layer(
    const Layer& red_layer,
    const Layer& green_layer,
    const Layer& blue_layer,
    const Layer* alpha_layer) {
  const Size size = red_layer.size();

  // Verify all layers have the same size
  if (green_layer.size().width != size.width ||
      green_layer.size().height != size.height ||
      blue_layer.size().width != size.width ||
      blue_layer.size().height != size.height) {
    // Return empty layer on size mismatch
    return std::make_unique<Layer>(Size{1, 1}, "Merged");
  }

  if (alpha_layer &&
      (alpha_layer->size().width != size.width ||
       alpha_layer->size().height != size.height)) {
    // Return empty layer on size mismatch
    return std::make_unique<Layer>(Size{1, 1}, "Merged");
  }

  auto merged = std::make_unique<Layer>(size, "Merged");
  uint8_t* dst = merged->buffer().data();

  const uint8_t* red_src = red_layer.buffer().data();
  const uint8_t* green_src = green_layer.buffer().data();
  const uint8_t* blue_src = blue_layer.buffer().data();
  const uint8_t* alpha_src = alpha_layer ? alpha_layer->buffer().data() : nullptr;

  const int pixel_count = size.width * size.height;
  for (int i = 0; i < pixel_count; ++i) {
    const int idx = i * 4;

    // Extract grayscale values (using R channel from each grayscale layer)
    dst[idx] = red_src[idx];          // R
    dst[idx + 1] = green_src[idx];    // G
    dst[idx + 2] = blue_src[idx];     // B
    dst[idx + 3] = alpha_src ? alpha_src[idx] : 255;  // A
  }

  return merged;
}

void split_document_channels(ImageDocument& doc) {
  if (doc.layer_count() == 0) {
    return;
  }

  // Use the active layer, or the first layer if none is active
  const Layer* source_layer = doc.active_layer();
  if (!source_layer && doc.layer_count() > 0) {
    source_layer = &doc.layer_at(0);
  }

  if (!source_layer) {
    return;
  }

  auto channel_layers = split_layer_to_channels(*source_layer, true);

  for (auto& layer : channel_layers) {
    doc.add_layer(layer->name());
    Layer& added = doc.layer_at(doc.layer_count() - 1);
    std::copy(layer->buffer().data(),
              layer->buffer().data() + layer->buffer().byte_size(),
              added.buffer().data());
  }
}

void merge_document_channels(ImageDocument& doc) {
  if (doc.layer_count() < 3) {
    return;
  }

  const Layer& red = doc.layer_at(0);
  const Layer& green = doc.layer_at(1);
  const Layer& blue = doc.layer_at(2);
  const Layer* alpha = doc.layer_count() >= 4 ? &doc.layer_at(3) : nullptr;

  auto merged = merge_channels_to_layer(red, green, blue, alpha);

  doc.add_layer("Merged Channels");
  Layer& added = doc.layer_at(doc.layer_count() - 1);
  std::copy(merged->buffer().data(),
            merged->buffer().data() + merged->buffer().byte_size(),
            added.buffer().data());
}

}  // namespace ps::core
