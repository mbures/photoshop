#include "ps/rendering/viewport.h"

#include <algorithm>
#include <cmath>

namespace ps::rendering {

namespace {
constexpr float kMinZoom = 0.01f;
constexpr float kMaxZoom = 64.0f;
constexpr float kZoomStep = 1.414213562f;  // sqrt(2)
}  // namespace

Viewport::Viewport() : viewport_size_(800, 600) {}

Viewport::Viewport(ViewportSize size) : viewport_size_(size) {}

void Viewport::set_viewport_size(ViewportSize size) {
  viewport_size_ = size;

  if (zoom_mode_ == ZoomMode::FitToWindow) {
    // Zoom will be recalculated when fit_to_window is called
  }
}

void Viewport::set_zoom(float zoom) {
  zoom_ = zoom;
  clamp_zoom();
  zoom_mode_ = ZoomMode::Custom;
}

void Viewport::zoom_in() {
  zoom_ *= kZoomStep;
  clamp_zoom();
  zoom_mode_ = ZoomMode::Custom;
}

void Viewport::zoom_out() {
  zoom_ /= kZoomStep;
  clamp_zoom();
  zoom_mode_ = ZoomMode::Custom;
}

void Viewport::set_zoom_mode(ZoomMode mode) {
  zoom_mode_ = mode;

  if (mode == ZoomMode::ActualPixels) {
    zoom_ = 1.0f;
  }
}

void Viewport::set_pan(float x, float y) {
  pan_offset_.x = x;
  pan_offset_.y = y;
}

void Viewport::pan_by(float dx, float dy) {
  pan_offset_.x += dx;
  pan_offset_.y += dy;
}

void Viewport::center_on_image(core::Size image_size) {
  const float image_width = static_cast<float>(image_size.width);
  const float image_height = static_cast<float>(image_size.height);
  const float viewport_width = static_cast<float>(viewport_size_.width);
  const float viewport_height = static_cast<float>(viewport_size_.height);

  pan_offset_.x = (viewport_width - image_width * zoom_) / 2.0f;
  pan_offset_.y = (viewport_height - image_height * zoom_) / 2.0f;
}

void Viewport::fit_to_window(core::Size image_size) {
  update_zoom_for_fit(image_size);
  center_on_image(image_size);
  zoom_mode_ = ZoomMode::FitToWindow;
}

ImagePoint Viewport::viewport_to_image(ViewportPoint vp) const {
  return ImagePoint((vp.x - pan_offset_.x) / zoom_,
                    (vp.y - pan_offset_.y) / zoom_);
}

ViewportPoint Viewport::image_to_viewport(ImagePoint ip) const {
  return ViewportPoint(ip.x * zoom_ + pan_offset_.x,
                      ip.y * zoom_ + pan_offset_.y);
}

bool Viewport::is_visible(ImagePoint ip, core::Size image_size) const {
  const ViewportPoint vp = image_to_viewport(ip);
  return vp.x >= 0 && vp.x < viewport_size_.width &&
         vp.y >= 0 && vp.y < viewport_size_.height;
}

void Viewport::reset() {
  zoom_ = 1.0f;
  pan_offset_ = ViewportPoint(0.0f, 0.0f);
  zoom_mode_ = ZoomMode::ActualPixels;
}

void Viewport::clamp_zoom() {
  zoom_ = std::clamp(zoom_, kMinZoom, kMaxZoom);
}

void Viewport::update_zoom_for_fit(core::Size image_size) {
  if (image_size.width <= 0 || image_size.height <= 0) {
    zoom_ = 1.0f;
    return;
  }

  const float image_width = static_cast<float>(image_size.width);
  const float image_height = static_cast<float>(image_size.height);
  const float viewport_width = static_cast<float>(viewport_size_.width);
  const float viewport_height = static_cast<float>(viewport_size_.height);

  const float zoom_x = viewport_width / image_width;
  const float zoom_y = viewport_height / image_height;

  zoom_ = std::min(zoom_x, zoom_y);
  clamp_zoom();
}

}  // namespace ps::rendering
