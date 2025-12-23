#pragma once

#include <string>

#include "ps/core/image_document.h"

namespace ps::io {

class ImageFormat {
 public:
  virtual ~ImageFormat() = default;

  virtual std::string name() const = 0;
  virtual bool can_read(const std::string& path) const = 0;
  virtual bool can_write(const std::string& path,
                         const ps::core::ImageDocument& document) const = 0;

  virtual ps::core::ImageDocument load(const std::string& path) const = 0;
  virtual void save(const std::string& path,
                    const ps::core::ImageDocument& document) const = 0;
};

std::string file_extension(const std::string& path);

}  // namespace ps::io
