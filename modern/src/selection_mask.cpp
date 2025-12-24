#include "ps/core/selection_mask.h"

#include <algorithm>
#include <cmath>

namespace ps::core {

SelectionMask::SelectionMask(Size size) { resize(size); }

void SelectionMask::resize(Size size) {
  size_ = size;
  mask_.assign(static_cast<std::size_t>(size_.width * size_.height), 0);
}

void SelectionMask::clear() {
  fill(0);
}

void SelectionMask::fill(std::uint8_t value) {
  std::fill(mask_.begin(), mask_.end(), value);
}

std::uint8_t SelectionMask::at(int x, int y) const {
  if (x < 0 || y < 0 || x >= size_.width || y >= size_.height) {
    return 0;
  }
  return mask_[index_for(x, y)];
}

void SelectionMask::set(int x, int y, std::uint8_t value) {
  if (x < 0 || y < 0 || x >= size_.width || y >= size_.height) {
    return;
  }
  mask_[index_for(x, y)] = value;
}

bool SelectionMask::has_selection() const {
  return std::any_of(mask_.begin(), mask_.end(),
                     [](std::uint8_t value) { return value > 0; });
}

void SelectionMask::fill_rect(int x, int y, int width, int height,
                              std::uint8_t value) {
  if (width <= 0 || height <= 0) {
    return;
  }

  const int x0 = std::clamp(x, 0, size_.width);
  const int y0 = std::clamp(y, 0, size_.height);
  const int x1 = std::clamp(x + width, 0, size_.width);
  const int y1 = std::clamp(y + height, 0, size_.height);

  for (int yy = y0; yy < y1; ++yy) {
    for (int xx = x0; xx < x1; ++xx) {
      set_unchecked(xx, yy, value);
    }
  }
}

void SelectionMask::fill_ellipse(int x, int y, int width, int height,
                                 std::uint8_t value) {
  if (width <= 0 || height <= 0) {
    return;
  }

  const float rx = width / 2.0f;
  const float ry = height / 2.0f;
  if (rx <= 0.0f || ry <= 0.0f) {
    return;
  }

  const float cx = x + rx;
  const float cy = y + ry;

  const int x0 = std::clamp(x, 0, size_.width);
  const int y0 = std::clamp(y, 0, size_.height);
  const int x1 = std::clamp(x + width, 0, size_.width);
  const int y1 = std::clamp(y + height, 0, size_.height);

  for (int yy = y0; yy < y1; ++yy) {
    for (int xx = x0; xx < x1; ++xx) {
      const float dx = (xx + 0.5f - cx) / rx;
      const float dy = (yy + 0.5f - cy) / ry;
      if (dx * dx + dy * dy <= 1.0f) {
        set_unchecked(xx, yy, value);
      }
    }
  }
}

void SelectionMask::invert() {
  for (auto& value : mask_) {
    value = static_cast<std::uint8_t>(255 - value);
  }
}

void SelectionMask::feather(int radius) {
  if (radius <= 0 || mask_.empty()) {
    return;
  }

  std::vector<std::uint8_t> temp(mask_.size(), 0);
  const int r = radius;
  const int w = size_.width;
  const int h = size_.height;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int sum = 0;
      int count = 0;
      for (int yy = std::max(0, y - r); yy <= std::min(h - 1, y + r); ++yy) {
        for (int xx = std::max(0, x - r); xx <= std::min(w - 1, x + r); ++xx) {
          sum += at_unchecked(xx, yy);
          ++count;
        }
      }
      temp[index_for(x, y)] = static_cast<std::uint8_t>(sum / count);
    }
  }

  mask_.swap(temp);
}

void SelectionMask::grow(int radius) {
  if (radius <= 0 || mask_.empty()) {
    return;
  }

  std::vector<std::uint8_t> result(mask_.size(), 0);
  const int r = radius;
  const int w = size_.width;
  const int h = size_.height;
  const int r_sq = r * r;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      bool selected = false;
      for (int yy = std::max(0, y - r); yy <= std::min(h - 1, y + r); ++yy) {
        for (int xx = std::max(0, x - r); xx <= std::min(w - 1, x + r); ++xx) {
          const int dx = xx - x;
          const int dy = yy - y;
          if (dx * dx + dy * dy <= r_sq && at_unchecked(xx, yy) > 0) {
            selected = true;
            break;
          }
        }
        if (selected) {
          break;
        }
      }
      result[index_for(x, y)] = selected ? 255 : 0;
    }
  }

  mask_.swap(result);
}

void SelectionMask::shrink(int radius) {
  if (radius <= 0 || mask_.empty()) {
    return;
  }

  std::vector<std::uint8_t> result(mask_.size(), 0);
  const int r = radius;
  const int w = size_.width;
  const int h = size_.height;
  const int r_sq = r * r;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (at_unchecked(x, y) == 0) {
        continue;
      }
      bool keep = true;
      for (int yy = std::max(0, y - r); yy <= std::min(h - 1, y + r); ++yy) {
        for (int xx = std::max(0, x - r); xx <= std::min(w - 1, x + r); ++xx) {
          const int dx = xx - x;
          const int dy = yy - y;
          if (dx * dx + dy * dy <= r_sq && at_unchecked(xx, yy) == 0) {
            keep = false;
            break;
          }
        }
        if (!keep) {
          break;
        }
      }
      result[index_for(x, y)] = keep ? 255 : 0;
    }
  }

  mask_.swap(result);
}

}  // namespace ps::core
