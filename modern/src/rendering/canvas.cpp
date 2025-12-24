#include "ps/rendering/canvas.h"

#include <algorithm>
#include <cmath>

#include "ps/core/layer_blend.h"

namespace ps::rendering {

Canvas::Canvas() : viewport_() {}

Canvas::Canvas(const Viewport& viewport) : viewport_(viewport) {}

void Canvas::render(const core::ImageDocument& doc, CanvasBuffer& buffer) {
  render_background(buffer);
  render_image(doc, buffer);
}

void Canvas::render_with_overlay(const core::ImageDocument& doc,
                                 CanvasBuffer& buffer,
                                 const SelectionOverlay& overlay) {
  render_background(buffer);
  render_image(doc, buffer);

  if (overlay.enabled) {
    render_selection_overlay(buffer, overlay);
  }
}

void Canvas::render_background(CanvasBuffer& buffer) {
  if (checkerboard_enabled_) {
    render_checkerboard(buffer);
  } else {
    buffer.clear(background_color_);
  }
}

void Canvas::render_checkerboard(CanvasBuffer& buffer, int checker_size) {
  const RGBAPixel light{200, 200, 200, 255};
  const RGBAPixel dark{150, 150, 150, 255};

  for (int y = 0; y < buffer.height; ++y) {
    for (int x = 0; x < buffer.width; ++x) {
      const int checker_x = x / checker_size;
      const int checker_y = y / checker_size;
      const bool is_light = (checker_x + checker_y) % 2 == 0;

      buffer.at(x, y) = is_light ? light : dark;
    }
  }
}

void Canvas::render_image(const core::ImageDocument& doc, CanvasBuffer& buffer) {
  const core::Size doc_size = doc.size();
  if (doc_size.width <= 0 || doc_size.height <= 0) {
    return;
  }

  // Use layer-based rendering if layers exist
  if (doc.layer_count() > 0) {
    render_layers(doc, buffer);
    return;
  }

  // Fall back to channel-based rendering
  for (int y = 0; y < buffer.height; ++y) {
    for (int x = 0; x < buffer.width; ++x) {
      const ViewportPoint vp(static_cast<float>(x), static_cast<float>(y));
      const ImagePoint ip = viewport_.viewport_to_image(vp);

      if (ip.x >= 0 && ip.x < doc_size.width &&
          ip.y >= 0 && ip.y < doc_size.height) {
        const RGBAPixel sampled = sample_image(doc, ip.x, ip.y);
        const RGBAPixel bg = buffer.at(x, y);
        buffer.at(x, y) = blend_pixels(bg, sampled);
      }
    }
  }
}

void Canvas::render_layers(const core::ImageDocument& doc, CanvasBuffer& buffer) {
  const core::Size doc_size = doc.size();

  for (int y = 0; y < buffer.height; ++y) {
    for (int x = 0; x < buffer.width; ++x) {
      const ViewportPoint vp(static_cast<float>(x), static_cast<float>(y));
      const ImagePoint ip = viewport_.viewport_to_image(vp);

      if (ip.x < 0 || ip.x >= doc_size.width ||
          ip.y < 0 || ip.y >= doc_size.height) {
        continue;
      }

      RGBAPixel composite = buffer.at(x, y);

      // Composite all visible layers from bottom to top
      for (std::size_t i = 0; i < doc.layer_count(); ++i) {
        const core::Layer& layer = doc.layer_at(i);

        if (!layer.visible()) {
          continue;
        }

        const RGBAPixel layer_pixel = sample_layer(layer, ip.x, ip.y);

        // Apply layer blending
        std::uint8_t r = composite.r;
        std::uint8_t g = composite.g;
        std::uint8_t b = composite.b;
        std::uint8_t a = composite.a;

        core::blend_pixel(
          layer_pixel.r, layer_pixel.g, layer_pixel.b, layer_pixel.a,
          r, g, b, a,
          layer.opacity(),
          layer.blend_mode()
        );

        composite = RGBAPixel(r, g, b, a);
      }

      buffer.at(x, y) = composite;
    }
  }
}

void Canvas::render_selection_overlay(CanvasBuffer& buffer,
                                      const SelectionOverlay& overlay) {
  if (!overlay.mask) {
    return;
  }

  const core::SelectionMask& mask = *overlay.mask;
  if (!mask.has_selection()) {
    return;
  }

  const core::Size mask_size = mask.size();
  const int frame_offset = overlay.animation_frame % 8;

  for (int y = 0; y < buffer.height; ++y) {
    for (int x = 0; x < buffer.width; ++x) {
      const ViewportPoint vp(static_cast<float>(x), static_cast<float>(y));
      const ImagePoint ip = viewport_.viewport_to_image(vp);
      const int ix = static_cast<int>(std::floor(ip.x));
      const int iy = static_cast<int>(std::floor(ip.y));

      if (ix < 0 || iy < 0 || ix >= mask_size.width || iy >= mask_size.height) {
        continue;
      }

      if (!mask.is_selected(ix, iy)) {
        continue;
      }

      const bool edge =
          !mask.is_selected(ix - 1, iy) ||
          !mask.is_selected(ix + 1, iy) ||
          !mask.is_selected(ix, iy - 1) ||
          !mask.is_selected(ix, iy + 1);

      if (!edge) {
        continue;
      }

      const bool show_pixel = ((ix + iy + frame_offset) / 4) % 2 == 0;
      if (show_pixel) {
        const RGBAPixel bg = buffer.at(x, y);
        buffer.at(x, y) = blend_pixels(bg, overlay.color);
      }
    }
  }
}

RGBAPixel Canvas::sample_layer(const core::Layer& layer, float x, float y) {
  const core::Size layer_size = layer.size();
  const int ix = std::clamp(static_cast<int>(x), 0, layer_size.width - 1);
  const int iy = std::clamp(static_cast<int>(y), 0, layer_size.height - 1);

  const int pixel_index = iy * layer_size.width + ix;
  const int byte_index = pixel_index * 4;  // RGBA8 format

  const std::uint8_t* data = layer.buffer().data();

  return RGBAPixel(
    data[byte_index],      // R
    data[byte_index + 1],  // G
    data[byte_index + 2],  // B
    data[byte_index + 3]   // A
  );
}

RGBAPixel Canvas::sample_image(const core::ImageDocument& doc, float x, float y) {
  const core::Size doc_size = doc.size();
  const int ix = std::clamp(static_cast<int>(x), 0, doc_size.width - 1);
  const int iy = std::clamp(static_cast<int>(y), 0, doc_size.height - 1);

  const auto& channels = doc.channels();
  if (channels.empty()) {
    return RGBAPixel(0, 0, 0, 255);
  }

  const core::ColorMode mode = doc.mode();
  const int pixel_index = iy * doc_size.width + ix;

  if (mode == core::ColorMode::Grayscale && channels.size() >= 1) {
    const auto* data = channels[0].buffer.data();
    const std::size_t bytes_per_pixel =
        core::bytes_per_pixel(channels[0].buffer.format());
    const std::uint8_t gray = data[pixel_index * bytes_per_pixel];
    return RGBAPixel(gray, gray, gray, 255);
  }

  if (mode == core::ColorMode::RGB && channels.size() >= 3) {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;

    for (std::size_t c = 0; c < std::min(channels.size(), std::size_t(4)); ++c) {
      const auto& channel = channels[c];
      const auto* data = channel.buffer.data();
      const std::size_t bytes_per_pixel =
          core::bytes_per_pixel(channel.buffer.format());
      const std::uint8_t value = data[pixel_index * bytes_per_pixel];

      switch (c) {
        case 0: r = value; break;
        case 1: g = value; break;
        case 2: b = value; break;
        case 3: a = value; break;
      }
    }

    return RGBAPixel(r, g, b, a);
  }

  if (mode == core::ColorMode::CMYK && channels.size() >= 4) {
    const auto* c_data = channels[0].buffer.data();
    const auto* m_data = channels[1].buffer.data();
    const auto* y_data = channels[2].buffer.data();
    const auto* k_data = channels[3].buffer.data();

    const std::size_t bytes_per_pixel =
        core::bytes_per_pixel(channels[0].buffer.format());
    const std::size_t idx = pixel_index * bytes_per_pixel;

    const std::uint8_t c = c_data[idx];
    const std::uint8_t m = m_data[idx];
    const std::uint8_t y_val = y_data[idx];
    const std::uint8_t k = k_data[idx];

    // Simple CMYK to RGB conversion
    const std::uint8_t r = static_cast<std::uint8_t>(
        (255 - c) * (255 - k) / 255);
    const std::uint8_t g = static_cast<std::uint8_t>(
        (255 - m) * (255 - k) / 255);
    const std::uint8_t b = static_cast<std::uint8_t>(
        (255 - y_val) * (255 - k) / 255);

    return RGBAPixel(r, g, b, 255);
  }

  return RGBAPixel(0, 0, 0, 255);
}

RGBAPixel Canvas::blend_pixels(RGBAPixel bottom, RGBAPixel top) {
  if (top.a == 255) {
    return top;
  }

  if (top.a == 0) {
    return bottom;
  }

  const float alpha = top.a / 255.0f;
  const float inv_alpha = 1.0f - alpha;

  return RGBAPixel(
      static_cast<std::uint8_t>(top.r * alpha + bottom.r * inv_alpha),
      static_cast<std::uint8_t>(top.g * alpha + bottom.g * inv_alpha),
      static_cast<std::uint8_t>(top.b * alpha + bottom.b * inv_alpha),
      std::max(top.a, bottom.a)
  );
}

}  // namespace ps::rendering
