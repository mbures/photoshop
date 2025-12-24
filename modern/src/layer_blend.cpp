#include "ps/core/layer_blend.h"

#include <algorithm>
#include <cmath>

namespace ps::core {

namespace {

// Convert uint8_t [0-255] to float [0.0-1.0]
inline float to_float(uint8_t val) {
  return val / 255.0f;
}

// Convert float [0.0-1.0] to uint8_t [0-255]
inline uint8_t to_byte(float val) {
  return static_cast<uint8_t>(std::clamp(val * 255.0f, 0.0f, 255.0f));
}

// Blend mode implementations (operate on normalized float values)

float blend_multiply(float src, float dst) {
  return src * dst;
}

float blend_screen(float src, float dst) {
  return 1.0f - (1.0f - src) * (1.0f - dst);
}

float blend_overlay(float src, float dst) {
  if (dst < 0.5f) {
    return 2.0f * src * dst;
  } else {
    return 1.0f - 2.0f * (1.0f - src) * (1.0f - dst);
  }
}

float blend_darken(float src, float dst) {
  return std::min(src, dst);
}

float blend_lighten(float src, float dst) {
  return std::max(src, dst);
}

float blend_color_dodge(float src, float dst) {
  if (src >= 1.0f) {
    return 1.0f;
  }
  return std::min(1.0f, dst / (1.0f - src));
}

float blend_color_burn(float src, float dst) {
  if (src <= 0.0f) {
    return 0.0f;
  }
  return 1.0f - std::min(1.0f, (1.0f - dst) / src);
}

float blend_hard_light(float src, float dst) {
  if (src < 0.5f) {
    return 2.0f * src * dst;
  } else {
    return 1.0f - 2.0f * (1.0f - src) * (1.0f - dst);
  }
}

float blend_soft_light(float src, float dst) {
  if (src < 0.5f) {
    return dst - (1.0f - 2.0f * src) * dst * (1.0f - dst);
  } else {
    float d = (dst < 0.25f) ?
      ((16.0f * dst - 12.0f) * dst + 4.0f) * dst :
      std::sqrt(dst);
    return dst + (2.0f * src - 1.0f) * (d - dst);
  }
}

float blend_difference(float src, float dst) {
  return std::abs(dst - src);
}

float blend_exclusion(float src, float dst) {
  return dst + src - 2.0f * dst * src;
}

}  // namespace

void blend_pixel(uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a,
                 uint8_t& dst_r, uint8_t& dst_g, uint8_t& dst_b, uint8_t& dst_a,
                 int opacity, BlendMode mode) {
  // Convert to float
  float sr = to_float(src_r);
  float sg = to_float(src_g);
  float sb = to_float(src_b);
  float sa = to_float(src_a) * (opacity / 100.0f);

  float dr = to_float(dst_r);
  float dg = to_float(dst_g);
  float db = to_float(dst_b);
  float da = to_float(dst_a);

  // Apply blend mode to get blended color
  float br, bg, bb;

  switch (mode) {
    case BlendMode::Normal:
      br = sr;
      bg = sg;
      bb = sb;
      break;

    case BlendMode::Multiply:
      br = blend_multiply(sr, dr);
      bg = blend_multiply(sg, dg);
      bb = blend_multiply(sb, db);
      break;

    case BlendMode::Screen:
      br = blend_screen(sr, dr);
      bg = blend_screen(sg, dg);
      bb = blend_screen(sb, db);
      break;

    case BlendMode::Overlay:
      br = blend_overlay(sr, dr);
      bg = blend_overlay(sg, dg);
      bb = blend_overlay(sb, db);
      break;

    case BlendMode::Darken:
      br = blend_darken(sr, dr);
      bg = blend_darken(sg, dg);
      bb = blend_darken(sb, db);
      break;

    case BlendMode::Lighten:
      br = blend_lighten(sr, dr);
      bg = blend_lighten(sg, dg);
      bb = blend_lighten(sb, db);
      break;

    case BlendMode::ColorDodge:
      br = blend_color_dodge(sr, dr);
      bg = blend_color_dodge(sg, dg);
      bb = blend_color_dodge(sb, db);
      break;

    case BlendMode::ColorBurn:
      br = blend_color_burn(sr, dr);
      bg = blend_color_burn(sg, dg);
      bb = blend_color_burn(sb, db);
      break;

    case BlendMode::HardLight:
      br = blend_hard_light(sr, dr);
      bg = blend_hard_light(sg, dg);
      bb = blend_hard_light(sb, db);
      break;

    case BlendMode::SoftLight:
      br = blend_soft_light(sr, dr);
      bg = blend_soft_light(sg, dg);
      bb = blend_soft_light(sb, db);
      break;

    case BlendMode::Difference:
      br = blend_difference(sr, dr);
      bg = blend_difference(sg, dg);
      bb = blend_difference(sb, db);
      break;

    case BlendMode::Exclusion:
      br = blend_exclusion(sr, dr);
      bg = blend_exclusion(sg, dg);
      bb = blend_exclusion(sb, db);
      break;

    default:
      br = sr;
      bg = sg;
      bb = sb;
      break;
  }

  // Composite with alpha
  const float out_alpha = sa + da * (1.0f - sa);

  if (out_alpha > 0.0f) {
    dr = (br * sa + dr * da * (1.0f - sa)) / out_alpha;
    dg = (bg * sa + dg * da * (1.0f - sa)) / out_alpha;
    db = (bb * sa + db * da * (1.0f - sa)) / out_alpha;
    da = out_alpha;
  }

  // Convert back to bytes
  dst_r = to_byte(dr);
  dst_g = to_byte(dg);
  dst_b = to_byte(db);
  dst_a = to_byte(da);
}

void composite_layer(const uint8_t* src_buffer, uint8_t* dst_buffer,
                     int width, int height, int opacity, BlendMode mode) {
  const int pixel_count = width * height;

  for (int i = 0; i < pixel_count; ++i) {
    const int idx = i * 4;

    blend_pixel(
      src_buffer[idx],     // src_r
      src_buffer[idx + 1], // src_g
      src_buffer[idx + 2], // src_b
      src_buffer[idx + 3], // src_a
      dst_buffer[idx],     // dst_r
      dst_buffer[idx + 1], // dst_g
      dst_buffer[idx + 2], // dst_b
      dst_buffer[idx + 3], // dst_a
      opacity,
      mode
    );
  }
}

}  // namespace ps::core
