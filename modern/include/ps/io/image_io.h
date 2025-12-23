#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ps/core/image_document.h"
#include "ps/io/image_format.h"

namespace ps::io {

class ImageIO {
 public:
  void register_format(std::unique_ptr<ImageFormat> format);

  ps::core::ImageDocument load(const std::string& path) const;
  void save(const std::string& path, const ps::core::ImageDocument& document) const;

 private:
  std::vector<std::unique_ptr<ImageFormat>> formats_{};
};

ImageIO create_default_image_io();

}  // namespace ps::io
