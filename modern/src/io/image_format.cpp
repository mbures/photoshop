#include "ps/io/image_format.h"

#include <algorithm>
#include <cctype>

namespace ps::io {

std::string file_extension(const std::string& path) {
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos || dot + 1 >= path.size()) {
    return {};
  }
  std::string extension = path.substr(dot + 1);
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char value) {
    return static_cast<char>(std::tolower(value));
  });
  return extension;
}

}  // namespace ps::io
