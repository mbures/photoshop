#include "ps/io/gif_format.h"

#include <algorithm>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ps::io {
namespace {

std::uint16_t read_le16(std::istream& stream) {
  std::uint8_t bytes[2] = {};
  stream.read(reinterpret_cast<char*>(bytes), 2);
  return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}

void write_le16(std::ostream& stream, std::uint16_t value) {
  std::uint8_t bytes[2] = {static_cast<std::uint8_t>(value & 0xFF),
                           static_cast<std::uint8_t>((value >> 8) & 0xFF)};
  stream.write(reinterpret_cast<const char*>(bytes), 2);
}

struct BitReader {
  const std::vector<std::uint8_t>& data;
  std::size_t byte_pos = 0;
  int bit_pos = 0;

  explicit BitReader(const std::vector<std::uint8_t>& data_ref) : data(data_ref) {}

  int read_bits(int count) {
    int value = 0;
    for (int i = 0; i < count; ++i) {
      if (byte_pos >= data.size()) {
        return -1;
      }
      if (data[byte_pos] & (1u << bit_pos)) {
        value |= (1 << i);
      }
      ++bit_pos;
      if (bit_pos == 8) {
        bit_pos = 0;
        ++byte_pos;
      }
    }
    return value;
  }
};

struct BitWriter {
  std::vector<std::uint8_t> data;
  int bit_pos = 0;

  void write_bits(int value, int count) {
    for (int i = 0; i < count; ++i) {
      if (bit_pos == 0) {
        data.push_back(0);
      }
      if (value & (1 << i)) {
        data.back() |= static_cast<std::uint8_t>(1u << bit_pos);
      }
      ++bit_pos;
      if (bit_pos == 8) {
        bit_pos = 0;
      }
    }
  }
};

struct LzwTableEntry {
  int prefix = -1;
  int final = 0;
  int son = -1;
  int brother = -1;
};

struct GifLzwTable {
  std::array<LzwTableEntry, 4096> table{};
  int next_code = 0;
  int word_size = 0;

  void init(int code_size) {
    word_size = code_size + 1;
    next_code = (1 << code_size) + 2;
    for (int code = 0; code < next_code - 2; ++code) {
      table[code].prefix = -1;
      table[code].final = code;
      table[code].son = -1;
      table[code].brother = -1;
    }
  }

  int search(int w, int k) {
    int code = table[w].son;
    while (code != -1) {
      if (table[code].final == k) {
        return code;
      }
      code = table[code].brother;
    }
    return -1;
  }

  void add(int w, int k, bool reading) {
    const int old_son = table[w].son;
    table[w].son = next_code;
    table[next_code].prefix = w;
    table[next_code].final = k;
    table[next_code].son = -1;
    table[next_code].brother = old_son;

    if (reading) {
      ++next_code;
      if (next_code == (1 << word_size) && word_size < 12) {
        ++word_size;
      }
    } else {
      if (next_code == (1 << word_size) && word_size < 12) {
        ++word_size;
      }
      ++next_code;
    }
  }
};

std::vector<std::uint8_t> read_sub_blocks(std::istream& stream) {
  std::vector<std::uint8_t> data;
  while (true) {
    std::uint8_t block_size = 0;
    stream.read(reinterpret_cast<char*>(&block_size), 1);
    if (!stream) {
      throw std::runtime_error("unexpected end of GIF data blocks");
    }
    if (block_size == 0) {
      break;
    }
    const std::size_t start = data.size();
    data.resize(start + block_size);
    stream.read(reinterpret_cast<char*>(data.data() + start), block_size);
    if (!stream) {
      throw std::runtime_error("unexpected end of GIF data blocks");
    }
  }
  return data;
}

std::vector<int> interlace_rows(int height) {
  std::vector<int> rows;
  rows.reserve(height);
  for (int pass = 0; pass < 4; ++pass) {
    int start = 0;
    int step = 0;
    switch (pass) {
      case 0:
        start = 0;
        step = 8;
        break;
      case 1:
        start = 4;
        step = 8;
        break;
      case 2:
        start = 2;
        step = 4;
        break;
      default:
        start = 1;
        step = 2;
        break;
    }
    for (int row = start; row < height; row += step) {
      rows.push_back(row);
    }
  }
  return rows;
}

std::vector<std::uint8_t> lzw_decode_gif(const std::vector<std::uint8_t>& data,
                                         int min_code_size,
                                         int width,
                                         int height,
                                         bool interlaced) {
  const int clear_code = 1 << min_code_size;
  const int end_code = clear_code + 1;

  GifLzwTable table;
  table.init(min_code_size);

  BitReader reader(data);
  std::vector<std::uint8_t> output(static_cast<std::size_t>(width) * height);
  std::vector<int> row_order = interlaced ? interlace_rows(height) : std::vector<int>();

  int old_code = -1;
  int fin_char = 0;
  int out_pos = 0;

  while (true) {
    int code = reader.read_bits(table.word_size);
    if (code < 0) {
      break;
    }
    if (code == clear_code) {
      table.init(min_code_size);
      old_code = -1;
      continue;
    }
    if (code == end_code) {
      break;
    }

    int in_code = code;
    std::array<int, 4096> stack{};
    int stack_count = 0;

    if (code >= table.next_code) {
      if (code != table.next_code || old_code == -1) {
        throw std::runtime_error("invalid GIF LZW code");
      }
      stack[stack_count++] = fin_char;
      code = old_code;
    }

    while (code >= clear_code) {
      stack[stack_count++] = table.table[code].final;
      code = table.table[code].prefix;
    }

    fin_char = code;

    auto write_pixel = [&](int value) {
      if (out_pos >= width * height) {
        return;
      }
      int row = out_pos / width;
      int col = out_pos % width;
      if (interlaced) {
        row = row_order[row];
      }
      output[static_cast<std::size_t>(row) * width + col] =
          static_cast<std::uint8_t>(value);
      ++out_pos;
    };

    write_pixel(code);
    for (int i = stack_count - 1; i >= 0; --i) {
      write_pixel(stack[i]);
    }

    if (old_code != -1 && table.next_code < 4096) {
      table.add(old_code, fin_char, true);
    }
    old_code = in_code;
  }

  return output;
}

std::vector<std::uint8_t> lzw_encode_gif(const std::vector<std::uint8_t>& indices,
                                         int min_code_size) {
  GifLzwTable table;
  table.init(min_code_size);

  const int clear_code = 1 << min_code_size;
  const int end_code = clear_code + 1;

  BitWriter writer;
  writer.write_bits(clear_code, table.word_size);

  int code = -1;
  for (std::size_t i = 0; i < indices.size(); ++i) {
    const int pixel = indices[i];
    if (code == -1) {
      code = pixel;
      continue;
    }
    const int new_code = table.search(code, pixel);
    if (new_code == -1) {
      writer.write_bits(code, table.word_size);
      if (table.next_code < 4096) {
        table.add(code, pixel, false);
      } else {
        writer.write_bits(clear_code, table.word_size);
        table.init(min_code_size);
      }
      code = pixel;
    } else {
      code = new_code;
    }
  }

  if (code != -1) {
    writer.write_bits(code, table.word_size);
  }

  writer.write_bits(end_code, table.word_size);
  return writer.data;
}

std::vector<std::uint8_t> build_palette_332() {
  std::vector<std::uint8_t> palette(256 * 3);
  for (int r = 0; r < 8; ++r) {
    for (int g = 0; g < 8; ++g) {
      for (int b = 0; b < 4; ++b) {
        const int index = (r << 5) | (g << 2) | b;
        palette[index * 3] = static_cast<std::uint8_t>((r * 255) / 7);
        palette[index * 3 + 1] = static_cast<std::uint8_t>((g * 255) / 7);
        palette[index * 3 + 2] = static_cast<std::uint8_t>((b * 255) / 3);
      }
    }
  }
  return palette;
}

std::vector<std::uint8_t> quantize_to_332(const ps::core::ImageBuffer& buffer) {
  const auto size = buffer.size();
  const auto format = buffer.format();
  const auto bytes_per_pixel = ps::core::bytes_per_pixel(format);
  std::vector<std::uint8_t> indices(static_cast<std::size_t>(size.width) * size.height);
  const auto* data = buffer.data();

  for (int y = 0; y < size.height; ++y) {
    for (int x = 0; x < size.width; ++x) {
      const std::size_t idx = static_cast<std::size_t>(y) * size.width + x;
      const std::size_t offset = idx * bytes_per_pixel;
      std::uint8_t r = 0;
      std::uint8_t g = 0;
      std::uint8_t b = 0;
      if (format == ps::core::PixelFormat::Gray8) {
        r = data[offset];
        g = data[offset];
        b = data[offset];
      } else {
        r = data[offset];
        g = data[offset + 1];
        b = data[offset + 2];
      }
      const int r_index = (r * 7) / 255;
      const int g_index = (g * 7) / 255;
      const int b_index = (b * 3) / 255;
      indices[idx] = static_cast<std::uint8_t>((r_index << 5) | (g_index << 2) | b_index);
    }
  }

  return indices;
}

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

}  // namespace

std::string GIFFormat::name() const {
  return "GIF";
}

bool GIFFormat::can_read(const std::string& path) const {
  return file_extension(path) == "gif";
}

bool GIFFormat::can_write(const std::string& path,
                          const ps::core::ImageDocument& document) const {
  if (file_extension(path) != "gif") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  if (format != ps::core::PixelFormat::Gray8 && format != ps::core::PixelFormat::RGB8 &&
      format != ps::core::PixelFormat::RGBA8) {
    return false;
  }
  const auto size = document.channels().front().buffer.size();
  return size.width > 0 && size.height > 0 && size.width <= 65535 && size.height <= 65535;
}

ps::core::ImageDocument GIFFormat::load(const std::string& path) const {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to open GIF file");
  }

  char header[6] = {};
  stream.read(header, 6);
  if (!stream || (std::string(header, 6) != "GIF87a" && std::string(header, 6) != "GIF89a")) {
    throw std::runtime_error("invalid GIF header");
  }

  const int canvas_width = static_cast<int>(read_le16(stream));
  const int canvas_height = static_cast<int>(read_le16(stream));
  const std::uint8_t packed = static_cast<std::uint8_t>(stream.get());
  const bool global_table = (packed & 0x80u) != 0;
  const int global_table_size = 1 << ((packed & 0x07u) + 1);
  stream.get();  // background
  stream.get();  // aspect

  std::vector<std::uint8_t> global_palette;
  if (global_table) {
    global_palette.resize(static_cast<std::size_t>(global_table_size) * 3);
    stream.read(reinterpret_cast<char*>(global_palette.data()),
                static_cast<std::streamsize>(global_palette.size()));
  }

  bool found_image = false;
  bool interlaced = false;
  int image_width = 0;
  int image_height = 0;
  int transparent_index = -1;
  std::vector<std::uint8_t> palette = global_palette;
  std::vector<std::uint8_t> lzw_data;
  int min_code_size = 0;

  while (stream && !found_image) {
    const int separator = stream.get();
    if (separator == 0x2C) {
      read_le16(stream);  // left
      read_le16(stream);  // top
      image_width = static_cast<int>(read_le16(stream));
      image_height = static_cast<int>(read_le16(stream));
      const std::uint8_t img_packed = static_cast<std::uint8_t>(stream.get());
      interlaced = (img_packed & 0x40u) != 0;
      const bool local_table = (img_packed & 0x80u) != 0;
      if (local_table) {
        const int local_size = 1 << ((img_packed & 0x07u) + 1);
        palette.resize(static_cast<std::size_t>(local_size) * 3);
        stream.read(reinterpret_cast<char*>(palette.data()),
                    static_cast<std::streamsize>(palette.size()));
      }
      min_code_size = stream.get();
      lzw_data = read_sub_blocks(stream);
      found_image = true;
    } else if (separator == 0x21) {
      const int label = stream.get();
      if (label == 0xF9) {
        const std::uint8_t block_size = static_cast<std::uint8_t>(stream.get());
        if (block_size == 4) {
          const std::uint8_t flags = static_cast<std::uint8_t>(stream.get());
          read_le16(stream);  // delay
          const std::uint8_t index = static_cast<std::uint8_t>(stream.get());
          if (flags & 0x01u) {
            transparent_index = index;
          }
          stream.get();
        } else {
          stream.seekg(block_size, std::ios::cur);
          read_sub_blocks(stream);
        }
      } else {
        read_sub_blocks(stream);
      }
    } else if (separator == 0x3B || separator == EOF) {
      break;
    } else {
      throw std::runtime_error("unexpected GIF block");
    }
  }

  if (!found_image) {
    throw std::runtime_error("no GIF image data found");
  }

  const auto indices = lzw_decode_gif(lzw_data, min_code_size, image_width, image_height,
                                      interlaced);

  ps::core::ImageDocument document({image_width, image_height}, ps::core::ColorMode::RGB);
  auto& channel = document.add_channel("Composite", ps::core::PixelFormat::RGBA8);
  auto* dst = channel.buffer.data();

  for (std::size_t i = 0; i < indices.size(); ++i) {
    const int palette_index = indices[i];
    const std::size_t color_offset = static_cast<std::size_t>(palette_index) * 3;
    const std::size_t dst_offset = i * 4;
    if (color_offset + 2 >= palette.size()) {
      throw std::runtime_error("invalid GIF palette index");
    }
    dst[dst_offset] = palette[color_offset];
    dst[dst_offset + 1] = palette[color_offset + 1];
    dst[dst_offset + 2] = palette[color_offset + 2];
    dst[dst_offset + 3] =
        (palette_index == transparent_index) ? 0 : static_cast<std::uint8_t>(255);
  }

  return document;
}

void GIFFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  const auto size = buffer.size();
  if (size.width <= 0 || size.height <= 0) {
    throw std::runtime_error("invalid GIF dimensions");
  }

  std::ofstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("unable to create GIF file");
  }

  const auto palette = build_palette_332();
  const auto indices = quantize_to_332(buffer);

  stream.write("GIF89a", 6);
  write_le16(stream, static_cast<std::uint16_t>(size.width));
  write_le16(stream, static_cast<std::uint16_t>(size.height));
  stream.put(static_cast<char>(0xF7));
  stream.put(0);
  stream.put(0);
  stream.write(reinterpret_cast<const char*>(palette.data()),
               static_cast<std::streamsize>(palette.size()));

  stream.put(static_cast<char>(0x2C));
  write_le16(stream, 0);
  write_le16(stream, 0);
  write_le16(stream, static_cast<std::uint16_t>(size.width));
  write_le16(stream, static_cast<std::uint16_t>(size.height));
  stream.put(0);

  const int min_code_size = 8;
  stream.put(static_cast<char>(min_code_size));

  const auto lzw_data = lzw_encode_gif(indices, min_code_size);
  std::size_t offset = 0;
  while (offset < lzw_data.size()) {
    const std::size_t chunk = std::min<std::size_t>(255, lzw_data.size() - offset);
    stream.put(static_cast<char>(chunk));
    stream.write(reinterpret_cast<const char*>(lzw_data.data() + offset),
                 static_cast<std::streamsize>(chunk));
    offset += chunk;
  }
  stream.put(0);
  stream.put(static_cast<char>(0x3B));
}

}  // namespace ps::io
