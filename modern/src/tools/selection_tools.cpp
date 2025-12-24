#include "ps/tools/selection_tools.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace ps::tools {
namespace {

Rect make_rect(Point a, Point b) {
  const int x0 = std::min(a.x, b.x);
  const int y0 = std::min(a.y, b.y);
  const int x1 = std::max(a.x, b.x);
  const int y1 = std::max(a.y, b.y);
  return Rect{x0, y0, x1 - x0 + 1, y1 - y0 + 1};
}

void fill_polygon(core::SelectionMask& mask, const std::vector<Point>& points) {
  if (points.size() < 3) {
    return;
  }

  int min_y = points.front().y;
  int max_y = points.front().y;
  for (const auto& pt : points) {
    min_y = std::min(min_y, pt.y);
    max_y = std::max(max_y, pt.y);
  }

  const core::Size size = mask.size();
  min_y = std::clamp(min_y, 0, size.height - 1);
  max_y = std::clamp(max_y, 0, size.height - 1);

  std::vector<float> intersections;
  for (int y = min_y; y <= max_y; ++y) {
    intersections.clear();
    for (std::size_t i = 0; i < points.size(); ++i) {
      const Point p1 = points[i];
      const Point p2 = points[(i + 1) % points.size()];

      if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
        const float t = static_cast<float>(y - p1.y) /
                        static_cast<float>(p2.y - p1.y);
        const float x = p1.x + t * static_cast<float>(p2.x - p1.x);
        intersections.push_back(x);
      }
    }

    std::sort(intersections.begin(), intersections.end());
    for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
      int x_start = static_cast<int>(std::floor(intersections[i]));
      int x_end = static_cast<int>(std::ceil(intersections[i + 1]));
      x_start = std::clamp(x_start, 0, size.width - 1);
      x_end = std::clamp(x_end, 0, size.width - 1);
      for (int x = x_start; x <= x_end; ++x) {
        mask.set(x, y, 255);
      }
    }
  }
}

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

}  // namespace

void RectangularMarqueeTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  anchor_ = pt;
  before_selection_ = doc.selection();
  stroke_active_ = true;
  update_selection(doc, pt);
}

void RectangularMarqueeTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }
  update_selection(doc, pt);
}

std::unique_ptr<core::Command> RectangularMarqueeTool::end_stroke(
    core::ImageDocument& doc) {
  if (!stroke_active_) {
    return nullptr;
  }
  stroke_active_ = false;
  core::SelectionMask after = doc.selection();
  return std::make_unique<core::SelectionCommand>(
      doc, std::move(before_selection_), std::move(after), "Rectangular Marquee");
}

void RectangularMarqueeTool::update_selection(core::ImageDocument& doc, Point pt) {
  Rect rect = make_rect(anchor_, pt);
  core::SelectionMask& selection = doc.selection();
  selection.clear();
  selection.fill_rect(rect.x, rect.y, rect.width, rect.height, 255);
}

void EllipticalMarqueeTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  anchor_ = pt;
  before_selection_ = doc.selection();
  stroke_active_ = true;
  update_selection(doc, pt);
}

void EllipticalMarqueeTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }
  update_selection(doc, pt);
}

std::unique_ptr<core::Command> EllipticalMarqueeTool::end_stroke(
    core::ImageDocument& doc) {
  if (!stroke_active_) {
    return nullptr;
  }
  stroke_active_ = false;
  core::SelectionMask after = doc.selection();
  return std::make_unique<core::SelectionCommand>(
      doc, std::move(before_selection_), std::move(after), "Elliptical Marquee");
}

void EllipticalMarqueeTool::update_selection(core::ImageDocument& doc, Point pt) {
  Rect rect = make_rect(anchor_, pt);
  core::SelectionMask& selection = doc.selection();
  selection.clear();
  selection.fill_ellipse(rect.x, rect.y, rect.width, rect.height, 255);
}

void LassoSelectionTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  before_selection_ = doc.selection();
  points_.clear();
  points_.push_back(pt);
  stroke_active_ = true;
  update_selection(doc);
}

void LassoSelectionTool::continue_stroke(core::ImageDocument& doc, Point pt) {
  if (!stroke_active_) {
    return;
  }
  points_.push_back(pt);
  update_selection(doc);
}

std::unique_ptr<core::Command> LassoSelectionTool::end_stroke(
    core::ImageDocument& doc) {
  if (!stroke_active_) {
    return nullptr;
  }
  stroke_active_ = false;
  core::SelectionMask after = doc.selection();
  return std::make_unique<core::SelectionCommand>(
      doc, std::move(before_selection_), std::move(after), "Lasso Selection");
}

void LassoSelectionTool::update_selection(core::ImageDocument& doc) {
  core::SelectionMask& selection = doc.selection();
  selection.clear();
  fill_polygon(selection, points_);
}

void MagicWandTool::begin_stroke(core::ImageDocument& doc, Point pt) {
  before_selection_ = doc.selection();
  after_selection_ = doc.selection();
  after_selection_.clear();
  stroke_active_ = true;

  const core::Size size = doc.size();
  if (pt.x < 0 || pt.y < 0 || pt.x >= size.width || pt.y >= size.height) {
    return;
  }

  const RGBColor target = sample_color(doc, pt.x, pt.y);
  const int tolerance = std::clamp(options_.size, 0, 255);

  std::vector<bool> visited(static_cast<std::size_t>(size.width * size.height), false);
  std::queue<Point> queue;
  queue.push(pt);

  while (!queue.empty()) {
    const Point current = queue.front();
    queue.pop();
    if (current.x < 0 || current.y < 0 ||
        current.x >= size.width || current.y >= size.height) {
      continue;
    }

    const int idx = current.y * size.width + current.x;
    if (visited[idx]) {
      continue;
    }
    visited[idx] = true;

    const RGBColor color = sample_color(doc, current.x, current.y);
    const int diff = std::abs(color.r - target.r) +
                     std::abs(color.g - target.g) +
                     std::abs(color.b - target.b);
    if (diff > tolerance) {
      continue;
    }

    after_selection_.set(current.x, current.y, 255);

    queue.push(Point(current.x + 1, current.y));
    queue.push(Point(current.x - 1, current.y));
    queue.push(Point(current.x, current.y + 1));
    queue.push(Point(current.x, current.y - 1));
  }

  doc.selection() = after_selection_;
}

void MagicWandTool::continue_stroke(core::ImageDocument&, Point) {}

std::unique_ptr<core::Command> MagicWandTool::end_stroke(
    core::ImageDocument& doc) {
  if (!stroke_active_) {
    return nullptr;
  }
  stroke_active_ = false;
  return std::make_unique<core::SelectionCommand>(
      doc, std::move(before_selection_), std::move(after_selection_), "Magic Wand");
}

}  // namespace ps::tools
