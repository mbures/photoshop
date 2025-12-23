#pragma once

#include <algorithm>
#include <cstddef>

#include "ps/core/image_document.h"

namespace ps::rendering {

struct ViewportPoint {
  float x = 0.0f;
  float y = 0.0f;

  ViewportPoint() = default;
  ViewportPoint(float x_, float y_) : x(x_), y(y_) {}
};

struct ImagePoint {
  float x = 0.0f;
  float y = 0.0f;

  ImagePoint() = default;
  ImagePoint(float x_, float y_) : x(x_), y(y_) {}
};

struct ViewportSize {
  int width = 0;
  int height = 0;

  ViewportSize() = default;
  ViewportSize(int w, int h) : width(w), height(h) {}
};

enum class ZoomMode {
  Custom,
  FitToWindow,
  ActualPixels
};

class Viewport {
 public:
  Viewport();
  explicit Viewport(ViewportSize size);

  void set_viewport_size(ViewportSize size);
  ViewportSize viewport_size() const { return viewport_size_; }

  void set_zoom(float zoom);
  float zoom() const { return zoom_; }

  void zoom_in();
  void zoom_out();
  void set_zoom_mode(ZoomMode mode);
  ZoomMode zoom_mode() const { return zoom_mode_; }

  void set_pan(float x, float y);
  void pan_by(float dx, float dy);
  ViewportPoint pan_offset() const { return pan_offset_; }

  void center_on_image(core::Size image_size);
  void fit_to_window(core::Size image_size);

  ImagePoint viewport_to_image(ViewportPoint vp) const;
  ViewportPoint image_to_viewport(ImagePoint ip) const;

  bool is_visible(ImagePoint ip, core::Size image_size) const;

  void reset();

 private:
  ViewportSize viewport_size_;
  float zoom_ = 1.0f;
  ViewportPoint pan_offset_{0.0f, 0.0f};
  ZoomMode zoom_mode_ = ZoomMode::ActualPixels;

  void clamp_zoom();
  void update_zoom_for_fit(core::Size image_size);
};

}  // namespace ps::rendering
