#pragma once

#include <vector>

#include "ps/core/image_document.h"
#include "ps/core/layer.h"

namespace ps::core {

/**
 * @brief Splits a layer into separate grayscale layers for each channel
 *
 * Creates new layers representing the R, G, B, and optionally A channels
 * of the source layer. Each resulting layer is grayscale.
 *
 * @param source The layer to split
 * @param include_alpha Whether to create a layer for the alpha channel
 * @return Vector of newly created layers (R, G, B, [A])
 */
std::vector<std::unique_ptr<Layer>> split_layer_to_channels(
    const Layer& source, bool include_alpha = false);

/**
 * @brief Merges separate channel layers into a single RGBA layer
 *
 * Combines grayscale layers representing R, G, B channels (and optionally A)
 * into a single RGBA layer.
 *
 * @param red_layer Layer containing red channel data
 * @param green_layer Layer containing green channel data
 * @param blue_layer Layer containing blue channel data
 * @param alpha_layer Optional layer containing alpha channel data (nullptr for opaque)
 * @return New merged layer
 */
std::unique_ptr<Layer> merge_channels_to_layer(
    const Layer& red_layer,
    const Layer& green_layer,
    const Layer& blue_layer,
    const Layer* alpha_layer = nullptr);

/**
 * @brief Splits the current image document into channel layers
 *
 * Creates new layers in the document, one for each color channel.
 * The original channels are preserved.
 *
 * @param doc The image document to operate on
 */
void split_document_channels(ImageDocument& doc);

/**
 * @brief Merges the first 3-4 layers as RGB(A) channels
 *
 * Takes the bottom 3 or 4 layers and merges them as color channels.
 *
 * @param doc The image document to operate on
 */
void merge_document_channels(ImageDocument& doc);

}  // namespace ps::core
