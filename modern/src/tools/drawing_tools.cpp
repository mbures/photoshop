#include "ps/tools/drawing_tools.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace ps::tools {
namespace {

struct RGBColor {
  int r = 0;
  int g = 0;
  int b = 0;
};

RGBColor sample_color(const core::ImageDocument& doc, int x, int y) {
  const auto& channels = doc.channels();
  const core::Size size = doc.size();
  if (channels.empty() || x < 0 || y < 0 || x >= size.width || y >= size.height) {
    return {};
  }

  const int index = y * size.width + x;
  const core::ColorMode mode = doc.mode();

  if (mode == core::ColorMode::Grayscale && channels.size() >= 1) {
    const auto* data = channels[0].buffer.data();
    const std::size_t bytes = core::bytes_per_pixel(channels[0].buffer.format());
    const std::uint8_t value = data[index * bytes];
    return {value, value, value};
  }

  if (mode == core::ColorMode::RGB && channels.size() >= 3) {
    const std::size_t bytes = core::bytes_per_pixel(channels[0].buffer.format());
    const std::uint8_t r = channels[0].buffer.data()[index * bytes];
    const std::uint8_t g = channels[1].buffer.data()[index * bytes];
    const std::uint8_t b = channels[2].buffer.data()[index * bytes];
    return {r, g, b};
  }

  if (mode == core::ColorMode::CMYK && channels.size() >= 4) {
    const std::size_t bytes = core::bytes_per_pixel(channels[0].buffer.format());
    const std::size_t idx = index * bytes;
    const std::uint8_t c = channels[0].buffer.data()[idx];
    const std::uint8_t m = channels[1].buffer.data()[idx];
    const std::uint8_t y_val = channels[2].buffer.data()[idx];
    const std::uint8_t k = channels[3].buffer.data()[idx];
    const std::uint8_t r = static_cast<std::uint8_t>((255 - c) * (255 - k) / 255);
    const std::uint8_t g = static_cast<std::uint8_t>((255 - m) * (255 - k) / 255);
    const std::uint8_t b = static_cast<std::uint8_t>((255 - y_val) * (255 - k) / 255);
    return {r, g, b};
  }

  return {};
}

class StrokeCommand : public core::ImageCommand {
 public:
  StrokeCommand(core::ImageDocument& doc, std::string name)
      : ImageCommand(doc), name_(std::move(name)) {
    save_all_channels();
  }

  void execute() override {}
  std::string name() const override { return name_; }

 private:
  std::string name_;
};

void apply_circular_dab(core::ImageDocument& doc, Point pt, int radius,
                        float hardness, float opacity, std::uint8_t target_value) {
  if (radius <= 0) {
    return;
  }

  const core::Size doc_size = doc.size();
  const int x_start = std::max(0, pt.x - radius);
  const int y_start = std::max(0, pt.y - radius);
  const int x_end = std::min(doc_size.width, pt.x + radius + 1);
  const int y_end = std::min(doc_size.height, pt.y + radius + 1);

  for (auto& channel : doc.channels()) {
    auto* data = channel.buffer.data();
    const int bytes_per_pixel =
        static_cast<int>(core::bytes_per_pixel(channel.buffer.format()));

    for (int y = y_start; y < y_end; ++y) {
      for (int x = x_start; x < x_end; ++x) {
        const int dx = x - pt.x;
        const int dy = y - pt.y;
        const float dist =
            std::sqrt(static_cast<float>(dx * dx + dy * dy)) / radius;

        if (dist > 1.0f) {
          continue;
        }

        float alpha = 1.0f;
        if (hardness < 1.0f && dist > hardness) {
          alpha = 1.0f - ((dist - hardness) / (1.0f - hardness));
        }
        alpha *= opacity;

        const int pixel_index = (y * doc_size.width + x) * bytes_per_pixel;

        for (int c = 0; c < bytes_per_pixel; ++c) {
          const std::uint8_t old_value = data[pixel_index + c];
          data[pixel_index + c] = static_cast<std::uint8_t>(
              old_value + alpha * (target_value - old_value));
        }
      }
    }
  }
}

void apply_bucket_fill(core::ImageDocument& doc, Point pt, int tolerance,
                       float opacity, std::uint8_t target_value) {
  const core::Size size = doc.size();
  if (doc.channels().empty() || pt.x < 0 || pt.y < 0 || pt.x >= size.width ||
      pt.y >= size.height) {
    return;
  }

  const RGBColor target_color = sample_color(doc, pt.x, pt.y);
  std::vector<bool> visited(static_cast<std::size_t>(size.width * size.height),
                            false);
  std::queue<Point> queue;
  queue.push(pt);

  while (!queue.empty()) {
    const Point current = queue.front();
    queue.pop();

    if (current.x < 0 || current.y < 0 || current.x >= size.width ||
        current.y >= size.height) {
      continue;
    }

    const int idx = current.y * size.width + current.x;
    if (visited[idx]) {
      continue;
    }
    visited[idx] = true;

    const RGBColor color = sample_color(doc, current.x, current.y);
    const int diff = std::abs(color.r - target_color.r) +
                     std::abs(color.g - target_color.g) +
                     std::abs(color.b - target_color.b);
    if (diff > tolerance) {
      continue;
    }

    for (auto& channel : doc.channels()) {
      auto* data = channel.buffer.data();
      const int bytes_per_pixel =
          static_cast<int>(core::bytes_per_pixel(channel.buffer.format()));
      const int pixel_index = idx * bytes_per_pixel;

      for (int c = 0; c < bytes_per_pixel; ++c) {
        const std::uint8_t old_value = data[pixel_index + c];
        data[pixel_index + c] = static_cast<std::uint8_t>(
            old_value + opacity * (target_value - old_value));
      }
    }

    queue.push(Point(current.x + 1, current.y));
    queue.push(Point(current.x - 1, current.y));
    queue.push(Point(current.x, current.y + 1));
    queue.push(Point(current.x, current.y - 1));
  }
}

}  // namespace

PencilTool::PencilTool() {
  options_.size = 6;
  options_.hardness = 100;
  options_.opacity = 100;
  options_.blend_mode = BlendMode::Normal;
}

void PencilTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  affected_area_ = Rect(pt.x, pt.y, 0, 0);
  stroke_active_ = true;
  current_command_ = std::make_unique<StrokeCommand>(doc, "Pencil Stroke");
  apply_pencil_dab(doc, pt);
}

void PencilTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }
  apply_pencil_dab(doc, pt);
}

std::unique_ptr<core::Command> PencilTool::end_stroke(core::ImageDocument&) {
  stroke_active_ = false;
  affected_area_ = Rect(0, 0, 0, 0);
  return std::move(current_command_);
}

void PencilTool::apply_pencil_dab(core::ImageDocument& doc, Point pt) {
  expand_affected_area(pt);
  const int radius = std::max(1, options_.size / 2);
  const float opacity = options_.opacity / 100.0f;
  apply_circular_dab(doc, pt, radius, 1.0f, opacity, 255);
}

void PencilTool::expand_affected_area(Point pt) {
  const Rect dab_rect = calculate_dab_rect(pt);

  if (affected_area_.is_empty()) {
    affected_area_ = dab_rect;
  } else {
    const int left = std::min(affected_area_.x, dab_rect.x);
    const int top = std::min(affected_area_.y, dab_rect.y);
    const int right = std::max(affected_area_.x + affected_area_.width,
                               dab_rect.x + dab_rect.width);
    const int bottom = std::max(affected_area_.y + affected_area_.height,
                                dab_rect.y + dab_rect.height);
    affected_area_ = Rect(left, top, right - left, bottom - top);
  }
}

Rect PencilTool::calculate_dab_rect(Point pt) const {
  const int radius = std::max(1, options_.size / 2);
  return Rect(pt.x - radius, pt.y - radius, radius * 2, radius * 2);
}

EraserTool::EraserTool() {
  options_.size = 12;
  options_.hardness = 50;
  options_.opacity = 100;
  options_.blend_mode = BlendMode::Normal;
}

void EraserTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  affected_area_ = Rect(pt.x, pt.y, 0, 0);
  stroke_active_ = true;
  current_command_ = std::make_unique<StrokeCommand>(doc, "Erase Stroke");
  apply_eraser_dab(doc, pt);
}

void EraserTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }
  apply_eraser_dab(doc, pt);
}

std::unique_ptr<core::Command> EraserTool::end_stroke(core::ImageDocument&) {
  stroke_active_ = false;
  affected_area_ = Rect(0, 0, 0, 0);
  return std::move(current_command_);
}

void EraserTool::apply_eraser_dab(core::ImageDocument& doc, Point pt) {
  expand_affected_area(pt);
  const int radius = std::max(1, options_.size / 2);
  const float hardness = options_.hardness / 100.0f;
  const float opacity = options_.opacity / 100.0f;
  apply_circular_dab(doc, pt, radius, hardness, opacity, 0);
}

void EraserTool::expand_affected_area(Point pt) {
  const Rect dab_rect = calculate_dab_rect(pt);

  if (affected_area_.is_empty()) {
    affected_area_ = dab_rect;
  } else {
    const int left = std::min(affected_area_.x, dab_rect.x);
    const int top = std::min(affected_area_.y, dab_rect.y);
    const int right = std::max(affected_area_.x + affected_area_.width,
                               dab_rect.x + dab_rect.width);
    const int bottom = std::max(affected_area_.y + affected_area_.height,
                                dab_rect.y + dab_rect.height);
    affected_area_ = Rect(left, top, right - left, bottom - top);
  }
}

Rect EraserTool::calculate_dab_rect(Point pt) const {
  const int radius = std::max(1, options_.size / 2);
  return Rect(pt.x - radius, pt.y - radius, radius * 2, radius * 2);
}

PaintBucketTool::PaintBucketTool() {
  options_.size = 24;
  options_.hardness = 100;
  options_.opacity = 100;
  options_.blend_mode = BlendMode::Normal;
}

void PaintBucketTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  stroke_active_ = true;
  current_command_ = std::make_unique<StrokeCommand>(doc, "Paint Bucket");

  const int tolerance = std::clamp(options_.size, 0, 255);
  const float opacity = options_.opacity / 100.0f;
  apply_bucket_fill(doc, pt, tolerance, opacity, 255);
}

void PaintBucketTool::continue_stroke(core::ImageDocument&, Point) {}

std::unique_ptr<core::Command> PaintBucketTool::end_stroke(core::ImageDocument&) {
  stroke_active_ = false;
  return std::move(current_command_);
}

void EyedropperTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  const RGBColor color = sample_color(doc, pt.x, pt.y);
  sampled_color_.r = static_cast<std::uint8_t>(color.r);
  sampled_color_.g = static_cast<std::uint8_t>(color.g);
  sampled_color_.b = static_cast<std::uint8_t>(color.b);
}

void EyedropperTool::continue_stroke(core::ImageDocument&, Point) {}

std::unique_ptr<core::Command> EyedropperTool::end_stroke(core::ImageDocument&) {
  return nullptr;
}

}  // namespace ps::tools
