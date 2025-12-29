#include "ps/io/bmp_format.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ps::io {
namespace {

constexpr std::uint16_t kBmpSignature = 0x4D42;  // "BM"

std::uint16_t read_le16(std::istream& stream) {
  std::uint8_t bytes[2] = {};
  stream.read(reinterpret_cast<char*>(bytes), 2);
  return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}

std::uint32_t read_le32(std::istream& stream) {
  std::uint8_t bytes[4] = {};
  stream.read(reinterpret_cast<char*>(bytes), 4);
  return static_cast<std::uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) |
                                    (bytes[3] << 24));
}

void write_le16(std::ostream& stream, std::uint16_t value) {
  std::uint8_t bytes[2] = {static_cast<std::uint8_t>(value & 0xFF),
                           static_cast<std::uint8_t>((value >> 8) & 0xFF)};
  stream.write(reinterpret_cast<const char*>(bytes), 2);
}

void write_le32(std::ostream& stream, std::uint32_t value) {
  std::uint8_t bytes[4] = {static_cast<std::uint8_t>(value & 0xFF),
                           static_cast<std::uint8_t>((value >> 8) & 0xFF),
                           static_cast<std::uint8_t>((value >> 16) & 0xFF),
                           static_cast<std::uint8_t>((value >> 24) & 0xFF)};
  stream.write(reinterpret_cast<const char*>(bytes), 4);
}

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

}  // namespace

std::string BMPFormat::name() const {
  return "BMP";
}

bool BMPFormat::can_read(const std::string& path) const {
  return file_extension(path) == "bmp";
}

bool BMPFormat::can_write(const std::string& path,
                          const ps::core::ImageDocument& document) const {
  if (file_extension(path) != "bmp") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  return format == ps::core::PixelFormat::Gray8 || format == ps::core::PixelFormat::RGB8 ||
         format == ps::core::PixelFormat::RGBA8;
}

ps::core::ImageDocument BMPFormat::load(const std::string& path) const {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to open BMP file");
  }

  const std::uint16_t signature = read_le16(stream);
  if (signature != kBmpSignature) {
    throw std::runtime_error("invalid BMP signature");
  }

  read_le32(stream);  // file size
  read_le16(stream);  // reserved1
  read_le16(stream);  // reserved2
  const std::uint32_t data_offset = read_le32(stream);

  const std::uint32_t header_size = read_le32(stream);
  if (header_size < 40) {
    throw std::runtime_error("unsupported BMP header size");
  }

  const std::int32_t width = static_cast<std::int32_t>(read_le32(stream));
  const std::int32_t height = static_cast<std::int32_t>(read_le32(stream));
  const std::uint16_t planes = read_le16(stream);
  const std::uint16_t bits_per_pixel = read_le16(stream);
  const std::uint32_t compression = read_le32(stream);
  read_le32(stream);  // image size
  read_le32(stream);  // x ppm
  read_le32(stream);  // y ppm
  read_le32(stream);  // colors used
  read_le32(stream);  // colors important

  if (planes != 1) {
    throw std::runtime_error("unsupported BMP planes value");
  }
  if (compression != 0) {
    throw std::runtime_error("compressed BMP files are not supported");
  }
  if (bits_per_pixel != 24 && bits_per_pixel != 32) {
    throw std::runtime_error("only 24-bit and 32-bit BMP files are supported");
  }

  const int abs_height = std::abs(height);
  const bool bottom_up = height > 0;

  ps::core::ImageDocument document({width, abs_height}, ps::core::ColorMode::RGB);
  const auto format = bits_per_pixel == 32 ? ps::core::PixelFormat::RGBA8
                                           : ps::core::PixelFormat::RGB8;
  auto& channel = document.add_channel("Composite", format);

  const std::size_t row_bytes_in_file =
      ((static_cast<std::size_t>(width) * bits_per_pixel + 31) / 32) * 4;
  const std::size_t row_bytes = static_cast<std::size_t>(width) *
                                ps::core::bytes_per_pixel(format);

  stream.seekg(static_cast<std::streamoff>(data_offset), std::ios::beg);
  std::vector<std::uint8_t> row(row_bytes_in_file);
  auto* dst = channel.buffer.data();

  for (int y = 0; y < abs_height; ++y) {
    const int dst_row = bottom_up ? (abs_height - 1 - y) : y;
    stream.read(reinterpret_cast<char*>(row.data()),
                static_cast<std::streamsize>(row.size()));
    if (!stream) {
      throw std::runtime_error("unexpected end of BMP data");
    }

    for (int x = 0; x < width; ++x) {
      const std::size_t src_index = static_cast<std::size_t>(x) * (bits_per_pixel / 8);
      const std::size_t dst_index =
          (static_cast<std::size_t>(dst_row) * width + x) *
          ps::core::bytes_per_pixel(format);
      const std::uint8_t b = row[src_index];
      const std::uint8_t g = row[src_index + 1];
      const std::uint8_t r = row[src_index + 2];
      dst[dst_index] = r;
      dst[dst_index + 1] = g;
      dst[dst_index + 2] = b;
      if (format == ps::core::PixelFormat::RGBA8) {
        dst[dst_index + 3] = (bits_per_pixel == 32) ? row[src_index + 3] : 255;
      }
    }
  }

  return document;
}

void BMPFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  const auto size = buffer.size();
  const auto format = buffer.format();

  std::ofstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to create BMP file");
  }

  const std::size_t row_bytes_unpadded = static_cast<std::size_t>(size.width) * 3;
  const std::size_t row_bytes = ((row_bytes_unpadded + 3) / 4) * 4;
  const std::uint32_t data_offset = 14 + 40;
  const std::uint32_t image_size = static_cast<std::uint32_t>(row_bytes * size.height);
  const std::uint32_t file_size = data_offset + image_size;

  write_le16(stream, kBmpSignature);
  write_le32(stream, file_size);
  write_le16(stream, 0);
  write_le16(stream, 0);
  write_le32(stream, data_offset);

  write_le32(stream, 40);  // BITMAPINFOHEADER size
  write_le32(stream, static_cast<std::uint32_t>(size.width));
  write_le32(stream, static_cast<std::uint32_t>(size.height));
  write_le16(stream, 1);
  write_le16(stream, 24);
  write_le32(stream, 0);
  write_le32(stream, image_size);
  write_le32(stream, 0);
  write_le32(stream, 0);
  write_le32(stream, 0);
  write_le32(stream, 0);

  std::vector<std::uint8_t> row(row_bytes, 0);
  const auto bytes_per_pixel = ps::core::bytes_per_pixel(format);

  for (int y = size.height - 1; y >= 0; --y) {
    std::fill(row.begin(), row.end(), 0);
    const std::uint8_t* src = buffer.data() +
                              static_cast<std::size_t>(y) * size.width * bytes_per_pixel;
    for (int x = 0; x < size.width; ++x) {
      std::uint8_t r = 0;
      std::uint8_t g = 0;
      std::uint8_t b = 0;
      if (format == ps::core::PixelFormat::Gray8) {
        const std::uint8_t value = src[x];
        r = value;
        g = value;
        b = value;
      } else if (format == ps::core::PixelFormat::RGB8) {
        const std::size_t idx = static_cast<std::size_t>(x) * 3;
        r = src[idx];
        g = src[idx + 1];
        b = src[idx + 2];
      } else {
        const std::size_t idx = static_cast<std::size_t>(x) * 4;
        r = src[idx];
        g = src[idx + 1];
        b = src[idx + 2];
      }
      const std::size_t dst_index = static_cast<std::size_t>(x) * 3;
      row[dst_index] = b;
      row[dst_index + 1] = g;
      row[dst_index + 2] = r;
    }
    stream.write(reinterpret_cast<const char*>(row.data()),
                 static_cast<std::streamsize>(row.size()));
  }
}

}  // namespace ps::io
