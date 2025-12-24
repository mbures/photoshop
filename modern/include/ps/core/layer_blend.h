#pragma once

#include <cstdint>

#include "ps/core/layer.h"

namespace ps::core {

/**
 * @brief Blends a single pixel from source layer onto destination
 *
 * Applies the specified blend mode to composite a source pixel (from a layer)
 * onto a destination pixel (from the composited result below).
 *
 * @param src_r Source red (0-255)
 * @param src_g Source green (0-255)
 * @param src_b Source blue (0-255)
 * @param src_a Source alpha (0-255)
 * @param dst_r Destination red (0-255), modified in place
 * @param dst_g Destination green (0-255), modified in place
 * @param dst_b Destination blue (0-255), modified in place
 * @param dst_a Destination alpha (0-255), modified in place
 * @param opacity Layer opacity (0-100)
 * @param mode Blend mode to apply
 */
void blend_pixel(uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a,
                 uint8_t& dst_r, uint8_t& dst_g, uint8_t& dst_b, uint8_t& dst_a,
                 int opacity, BlendMode mode);

/**
 * @brief Composites a layer onto a destination buffer
 *
 * Applies the layer's pixels to the destination using the layer's blend mode
 * and opacity. Both buffers must be RGBA8 format with matching dimensions.
 *
 * @param src_buffer Source layer buffer (RGBA8)
 * @param dst_buffer Destination buffer (RGBA8), modified in place
 * @param width Width in pixels
 * @param height Height in pixels
 * @param opacity Layer opacity (0-100)
 * @param mode Blend mode to apply
 */
void composite_layer(const uint8_t* src_buffer, uint8_t* dst_buffer,
                     int width, int height, int opacity, BlendMode mode);

}  // namespace ps::core
