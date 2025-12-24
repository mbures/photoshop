#include "ps/tools/brush_tool.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ps::tools {

// BrushStrokeCommand implementation

BrushStrokeCommand::BrushStrokeCommand(core::ImageDocument& doc,
                                        const Rect& affected_area)
    : ImageCommand(doc), affected_area_(affected_area) {
  // Save the current state of all channels BEFORE any modifications.
  // This "before" state will be restored when undo() is called.
  save_all_channels();
}

void BrushStrokeCommand::execute() {
  // The brush stroke was already applied during the user's interaction
  // (in begin_stroke/continue_stroke), so execute() doesn't need to
  // reapply it. This command exists primarily to enable undo/redo.
}

// BrushTool implementation

BrushTool::BrushTool() {
  options_.size = 10;
  options_.hardness = 100;
  options_.opacity = 100;
  options_.blend_mode = BlendMode::Normal;
}

void BrushTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  stroke_points_.clear();
  affected_area_ = Rect(pt.x, pt.y, 0, 0);
  stroke_active_ = true;

  // Create the undo command BEFORE making any modifications
  // This captures the "before" state for undo
  current_command_ = std::make_unique<BrushStrokeCommand>(doc, affected_area_);

  stroke_points_.push_back({pt, 100});
  apply_brush_dab(doc, pt, 100);
}

void BrushTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }

  stroke_points_.push_back({pt, 100});
  apply_brush_dab(doc, pt, 100);
}

std::unique_ptr<core::Command> BrushTool::end_stroke(core::ImageDocument& doc) {
  stroke_active_ = false;
  stroke_points_.clear();

  // Return the command that was created in begin_stroke()
  // If no stroke was started (shouldn't happen), return nullptr
  auto cmd = std::move(current_command_);

  // Clear the affected area for the next stroke
  affected_area_ = Rect(0, 0, 0, 0);

  return cmd;
}

void BrushTool::apply_brush_dab(core::ImageDocument& doc, Point pt,
                                 int pressure) {
  if (doc.channels().empty()) {
    return;
  }

  expand_affected_area(pt);

  const int radius = (options_.size * pressure) / 200;
  if (radius <= 0) {
    return;
  }

  const core::Size doc_size = doc.size();
  const int x_start = std::max(0, pt.x - radius);
  const int y_start = std::max(0, pt.y - radius);
  const int x_end = std::min(doc_size.width, pt.x + radius + 1);
  const int y_end = std::min(doc_size.height, pt.y + radius + 1);

  const float hardness = options_.hardness / 100.0f;
  const float opacity = (options_.opacity * pressure) / 10000.0f;

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
          const std::uint8_t new_value = 255;
          data[pixel_index + c] = static_cast<std::uint8_t>(
              old_value + alpha * (new_value - old_value));
        }
      }
    }
  }
}

void BrushTool::expand_affected_area(Point pt) {
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

Rect BrushTool::calculate_dab_rect(Point pt) const {
  const int radius = options_.size / 2;
  return Rect(pt.x - radius, pt.y - radius, radius * 2, radius * 2);
}

}  // namespace ps::tools
