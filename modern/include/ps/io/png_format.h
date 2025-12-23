#pragma once

#include "ps/io/image_format.h"

namespace ps::io {

class PNGFormat : public ImageFormat {
 public:
  std::string name() const override;
  bool can_read(const std::string& path) const override;
  bool can_write(const std::string& path,
                 const ps::core::ImageDocument& document) const override;

  ps::core::ImageDocument load(const std::string& path) const override;
  void save(const std::string& path,
            const ps::core::ImageDocument& document) const override;
};

}  // namespace ps::io
