#include "ps/io/psd_format.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ps::io {
namespace {

std::uint16_t read_be16(std::istream& stream) {
  std::uint8_t bytes[2] = {};
  stream.read(reinterpret_cast<char*>(bytes), 2);
  return static_cast<std::uint16_t>((bytes[0] << 8) | bytes[1]);
}

std::uint32_t read_be32(std::istream& stream) {
  std::uint8_t bytes[4] = {};
  stream.read(reinterpret_cast<char*>(bytes), 4);
  return static_cast<std::uint32_t>((bytes[0] << 24) | (bytes[1] << 16) |
                                    (bytes[2] << 8) | bytes[3]);
}

void write_be16(std::ostream& stream, std::uint16_t value) {
  std::uint8_t bytes[2] = {static_cast<std::uint8_t>((value >> 8) & 0xFF),
                           static_cast<std::uint8_t>(value & 0xFF)};
  stream.write(reinterpret_cast<const char*>(bytes), 2);
}

void write_be32(std::ostream& stream, std::uint32_t value) {
  std::uint8_t bytes[4] = {static_cast<std::uint8_t>((value >> 24) & 0xFF),
                           static_cast<std::uint8_t>((value >> 16) & 0xFF),
                           static_cast<std::uint8_t>((value >> 8) & 0xFF),
                           static_cast<std::uint8_t>(value & 0xFF)};
  stream.write(reinterpret_cast<const char*>(bytes), 4);
}

std::vector<std::uint8_t> read_bytes(std::istream& stream, std::size_t count) {
  std::vector<std::uint8_t> buffer(count);
  stream.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(count));
  if (!stream) {
    throw std::runtime_error("unexpected end of PSD file");
  }
  return buffer;
}

std::vector<std::uint8_t> packbits_decode(const std::vector<std::uint8_t>& data,
                                          std::size_t expected_size) {
  std::vector<std::uint8_t> output;
  output.reserve(expected_size);
  std::size_t offset = 0;
  while (offset < data.size() && output.size() < expected_size) {
    const std::int8_t header = static_cast<std::int8_t>(data[offset++]);
    if (header >= 0) {
      const int count = header + 1;
      if (offset + count > data.size()) {
        throw std::runtime_error("invalid PSD RLE data");
      }
      output.insert(output.end(), data.begin() + offset, data.begin() + offset + count);
      offset += count;
    } else if (header != -128) {
      const int count = 1 - header;
      if (offset >= data.size()) {
        throw std::runtime_error("invalid PSD RLE data");
      }
      output.insert(output.end(), count, data[offset]);
      ++offset;
    }
  }
  if (output.size() != expected_size) {
    throw std::runtime_error("PSD RLE data size mismatch");
  }
  return output;
}

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

}  // namespace

std::string PSDFormat::name() const {
  return "PSD";
}

bool PSDFormat::can_read(const std::string& path) const {
  return file_extension(path) == "psd";
}

bool PSDFormat::can_write(const std::string& path,
                          const ps::core::ImageDocument& document) const {
  if (file_extension(path) != "psd") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  return format == ps::core::PixelFormat::Gray8 || format == ps::core::PixelFormat::RGB8 ||
         format == ps::core::PixelFormat::RGBA8;
}

ps::core::ImageDocument PSDFormat::load(const std::string& path) const {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to open PSD file");
  }

  char signature[4] = {};
  stream.read(signature, 4);
  if (std::string(signature, 4) != "8BPS") {
    throw std::runtime_error("invalid PSD signature");
  }

  const std::uint16_t version = read_be16(stream);
  if (version != 1) {
    throw std::runtime_error("unsupported PSD version");
  }

  stream.seekg(6, std::ios::cur);

  const std::uint16_t channels = read_be16(stream);
  const std::uint32_t height = read_be32(stream);
  const std::uint32_t width = read_be32(stream);
  const std::uint16_t depth = read_be16(stream);
  const std::uint16_t color_mode = read_be16(stream);

  if (depth != 8) {
    throw std::runtime_error("only 8-bit PSD files are supported");
  }
  if (color_mode != 1 && color_mode != 3) {
    throw std::runtime_error("only Grayscale and RGB PSD files are supported");
  }
  if (channels < 1 || channels > 4) {
    throw std::runtime_error("unsupported PSD channel count");
  }

  const std::uint32_t color_data_length = read_be32(stream);
  stream.seekg(color_data_length, std::ios::cur);

  const std::uint32_t resources_length = read_be32(stream);
  stream.seekg(resources_length, std::ios::cur);

  const std::uint32_t layer_mask_length = read_be32(stream);
  stream.seekg(layer_mask_length, std::ios::cur);

  const std::uint16_t compression = read_be16(stream);

  const std::size_t plane_size = static_cast<std::size_t>(width) * height;
  std::vector<std::vector<std::uint8_t>> planes(channels);

  if (compression == 0) {
    for (std::uint16_t c = 0; c < channels; ++c) {
      planes[c] = read_bytes(stream, plane_size);
    }
  } else if (compression == 1) {
    std::vector<std::uint16_t> row_lengths(channels * height);
    for (std::size_t i = 0; i < row_lengths.size(); ++i) {
      row_lengths[i] = read_be16(stream);
    }
    for (std::uint16_t c = 0; c < channels; ++c) {
      std::vector<std::uint8_t> plane;
      plane.reserve(plane_size);
      for (std::uint32_t row = 0; row < height; ++row) {
        const std::uint16_t length = row_lengths[c * height + row];
        const auto row_data = read_bytes(stream, length);
        const auto decoded = packbits_decode(row_data, width);
        plane.insert(plane.end(), decoded.begin(), decoded.end());
      }
      planes[c] = std::move(plane);
    }
  } else {
    throw std::runtime_error("unsupported PSD compression");
  }

  const bool has_alpha = channels == 4;
  ps::core::PixelFormat format = ps::core::PixelFormat::RGB8;
  if (color_mode == 1) {
    format = ps::core::PixelFormat::Gray8;
  } else if (has_alpha) {
    format = ps::core::PixelFormat::RGBA8;
  }

  ps::core::ImageDocument document({static_cast<int>(width), static_cast<int>(height)},
                                   color_mode == 1 ? ps::core::ColorMode::Grayscale
                                                   : ps::core::ColorMode::RGB);
  auto& channel = document.add_channel("Composite", format);
  auto* dst = channel.buffer.data();

  for (std::size_t i = 0; i < plane_size; ++i) {
    if (format == ps::core::PixelFormat::Gray8) {
      dst[i] = planes[0][i];
    } else if (format == ps::core::PixelFormat::RGB8) {
      const std::size_t idx = i * 3;
      dst[idx] = planes[0][i];
      dst[idx + 1] = planes[1][i];
      dst[idx + 2] = planes[2][i];
    } else {
      const std::size_t idx = i * 4;
      dst[idx] = planes[0][i];
      dst[idx + 1] = planes[1][i];
      dst[idx + 2] = planes[2][i];
      dst[idx + 3] = planes[3][i];
    }
  }

  return document;
}

void PSDFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  const auto size = buffer.size();
  const auto format = buffer.format();

  std::ofstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to create PSD file");
  }

  std::uint16_t channels = 0;
  std::uint16_t color_mode = 3;
  if (format == ps::core::PixelFormat::Gray8) {
    channels = 1;
    color_mode = 1;
  } else if (format == ps::core::PixelFormat::RGB8) {
    channels = 3;
  } else {
    channels = 4;
  }

  stream.write("8BPS", 4);
  write_be16(stream, 1);
  std::array<std::uint8_t, 6> reserved{};
  stream.write(reinterpret_cast<const char*>(reserved.data()),
               static_cast<std::streamsize>(reserved.size()));
  write_be16(stream, channels);
  write_be32(stream, static_cast<std::uint32_t>(size.height));
  write_be32(stream, static_cast<std::uint32_t>(size.width));
  write_be16(stream, 8);
  write_be16(stream, color_mode);

  write_be32(stream, 0);
  write_be32(stream, 0);
  write_be32(stream, 0);

  write_be16(stream, 0);

  const std::size_t plane_size = static_cast<std::size_t>(size.width) * size.height;
  std::vector<std::uint8_t> plane_buffer(plane_size);

  for (std::uint16_t c = 0; c < channels; ++c) {
    for (std::size_t i = 0; i < plane_size; ++i) {
      if (format == ps::core::PixelFormat::Gray8) {
        plane_buffer[i] = buffer.data()[i];
      } else if (format == ps::core::PixelFormat::RGB8) {
        plane_buffer[i] = buffer.data()[i * 3 + c];
      } else {
        plane_buffer[i] = buffer.data()[i * 4 + c];
      }
    }
    stream.write(reinterpret_cast<const char*>(plane_buffer.data()),
                 static_cast<std::streamsize>(plane_buffer.size()));
  }
}

}  // namespace ps::io
