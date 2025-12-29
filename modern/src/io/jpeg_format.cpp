#include "ps/io/jpeg_format.h"

#include <csetjmp>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include <jpeglib.h>

namespace ps::io {
namespace {

struct JpegErrorManager {
  jpeg_error_mgr pub;
  std::jmp_buf setjmp_buffer;
  char message[JMSG_LENGTH_MAX] = {};
};

void jpeg_error_exit(j_common_ptr cinfo) {
  auto* err = reinterpret_cast<JpegErrorManager*>(cinfo->err);
  (*cinfo->err->format_message)(cinfo, err->message);
  std::longjmp(err->setjmp_buffer, 1);
}

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

}  // namespace

std::string JPEGFormat::name() const {
  return "JPEG";
}

bool JPEGFormat::can_read(const std::string& path) const {
  const auto ext = file_extension(path);
  return ext == "jpg" || ext == "jpeg";
}

bool JPEGFormat::can_write(const std::string& path,
                           const ps::core::ImageDocument& document) const {
  const auto ext = file_extension(path);
  if (ext != "jpg" && ext != "jpeg") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  return format == ps::core::PixelFormat::Gray8 || format == ps::core::PixelFormat::RGB8;
}

ps::core::ImageDocument JPEGFormat::load(const std::string& path) const {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (!file) {
    throw std::runtime_error("unable to open JPEG file");
  }

  jpeg_decompress_struct cinfo;
  JpegErrorManager jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    std::fclose(file);
    throw std::runtime_error(jerr.message);
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);
  cinfo.out_color_space = (cinfo.num_components == 1) ? JCS_GRAYSCALE : JCS_RGB;
  jpeg_start_decompress(&cinfo);

  const int width = static_cast<int>(cinfo.output_width);
  const int height = static_cast<int>(cinfo.output_height);
  const int components = static_cast<int>(cinfo.output_components);

  ps::core::PixelFormat format = (components == 1) ? ps::core::PixelFormat::Gray8
                                                   : ps::core::PixelFormat::RGB8;
  ps::core::ImageDocument document({width, height},
                                   components == 1 ? ps::core::ColorMode::Grayscale
                                                   : ps::core::ColorMode::RGB);
  auto& channel = document.add_channel("Composite", format);

  const std::size_t row_stride = static_cast<std::size_t>(width) * components;
  std::vector<std::uint8_t> row(row_stride);

  auto* dst = channel.buffer.data();
  std::size_t offset = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
    JSAMPROW row_pointer = row.data();
    jpeg_read_scanlines(&cinfo, &row_pointer, 1);
    std::copy(row.begin(), row.end(), dst + offset);
    offset += row_stride;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  std::fclose(file);

  return document;
}

void JPEGFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  FILE* file = std::fopen(path.c_str(), "wb");
  if (!file) {
    throw std::runtime_error("unable to create JPEG file");
  }

  jpeg_compress_struct cinfo;
  JpegErrorManager jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_compress(&cinfo);
    std::fclose(file);
    throw std::runtime_error(jerr.message);
  }

  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, file);

  const auto size = buffer.size();
  const auto format = buffer.format();
  cinfo.image_width = static_cast<JDIMENSION>(size.width);
  cinfo.image_height = static_cast<JDIMENSION>(size.height);
  cinfo.input_components = (format == ps::core::PixelFormat::Gray8) ? 1 : 3;
  cinfo.in_color_space = (format == ps::core::PixelFormat::Gray8) ? JCS_GRAYSCALE : JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, 90, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  const std::size_t row_stride = static_cast<std::size_t>(size.width) *
                                 static_cast<std::size_t>(cinfo.input_components);

  while (cinfo.next_scanline < cinfo.image_height) {
    const std::uint8_t* row = buffer.data() +
                              static_cast<std::size_t>(cinfo.next_scanline) * row_stride;
    JSAMPROW row_pointer = const_cast<JSAMPROW>(row);
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  std::fclose(file);
}

}  // namespace ps::io
