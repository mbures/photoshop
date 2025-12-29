#include "ps/io/tiff_format.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <tiffio.h>

namespace ps::io {
namespace {

const ps::core::ImageBuffer& primary_buffer(const ps::core::ImageDocument& document) {
  if (document.channels().empty()) {
    throw std::runtime_error("image document has no channels");
  }
  return document.channels().front().buffer;
}

ps::core::PixelFormat format_for_tiff(std::uint16_t samples) {
  switch (samples) {
    case 1:
      return ps::core::PixelFormat::Gray8;
    case 3:
      return ps::core::PixelFormat::RGB8;
    case 4:
      return ps::core::PixelFormat::RGBA8;
    default:
      break;
  }
  throw std::runtime_error("unsupported TIFF sample count");
}

}  // namespace

std::string TIFFFormat::name() const {
  return "TIFF";
}

bool TIFFFormat::can_read(const std::string& path) const {
  const auto ext = file_extension(path);
  return ext == "tif" || ext == "tiff";
}

bool TIFFFormat::can_write(const std::string& path,
                           const ps::core::ImageDocument& document) const {
  const auto ext = file_extension(path);
  if (ext != "tif" && ext != "tiff") {
    return false;
  }
  if (document.channels().empty()) {
    return false;
  }
  const auto format = document.channels().front().buffer.format();
  return format == ps::core::PixelFormat::Gray8 || format == ps::core::PixelFormat::RGB8 ||
         format == ps::core::PixelFormat::RGBA8;
}

ps::core::ImageDocument TIFFFormat::load(const std::string& path) const {
  TIFF* tif = TIFFOpen(path.c_str(), "r");
  if (!tif) {
    throw std::runtime_error("unable to open TIFF file");
  }

  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint16_t samples = 0;
  std::uint16_t bits_per_sample = 0;
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

  if (width == 0 || height == 0) {
    TIFFClose(tif);
    throw std::runtime_error("invalid TIFF dimensions");
  }
  if (bits_per_sample != 8) {
    TIFFClose(tif);
    throw std::runtime_error("only 8-bit TIFF files are supported");
  }

  std::vector<std::uint32_t> raster(width * height);
  if (!TIFFReadRGBAImageOriented(tif, width, height, raster.data(), ORIENTATION_TOPLEFT, 0)) {
    TIFFClose(tif);
    throw std::runtime_error("failed to read TIFF pixels");
  }
  TIFFClose(tif);

  const auto pixel_format = format_for_tiff(samples == 0 ? 4 : samples);
  ps::core::ImageDocument document(
      {static_cast<int>(width), static_cast<int>(height)},
      pixel_format == ps::core::PixelFormat::Gray8 ? ps::core::ColorMode::Grayscale
                                                   : ps::core::ColorMode::RGB);
  auto& channel = document.add_channel("Composite", pixel_format);

  auto* dst = channel.buffer.data();
  for (std::size_t i = 0; i < raster.size(); ++i) {
    const std::uint32_t pixel = raster[i];
    const std::uint8_t r = TIFFGetR(pixel);
    const std::uint8_t g = TIFFGetG(pixel);
    const std::uint8_t b = TIFFGetB(pixel);
    const std::uint8_t a = TIFFGetA(pixel);
    if (pixel_format == ps::core::PixelFormat::Gray8) {
      dst[i] = static_cast<std::uint8_t>((static_cast<int>(r) + g + b) / 3);
    } else if (pixel_format == ps::core::PixelFormat::RGB8) {
      const std::size_t idx = i * 3;
      dst[idx] = r;
      dst[idx + 1] = g;
      dst[idx + 2] = b;
    } else {
      const std::size_t idx = i * 4;
      dst[idx] = r;
      dst[idx + 1] = g;
      dst[idx + 2] = b;
      dst[idx + 3] = a;
    }
  }

  return document;
}

void TIFFFormat::save(const std::string& path, const ps::core::ImageDocument& document) const {
  const auto& buffer = primary_buffer(document);
  TIFF* tif = TIFFOpen(path.c_str(), "w");
  if (!tif) {
    throw std::runtime_error("unable to create TIFF file");
  }

  const auto size = buffer.size();
  const auto format = buffer.format();
  const std::uint16_t samples = (format == ps::core::PixelFormat::Gray8) ? 1
                              : (format == ps::core::PixelFormat::RGB8) ? 3
                                                                        : 4;

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, static_cast<std::uint32_t>(size.width));
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, static_cast<std::uint32_t>(size.height));
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samples);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

  if (samples == 1) {
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  } else {
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  }

  if (samples == 4) {
    std::uint16_t extrasample = EXTRASAMPLE_UNASSALPHA;
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &extrasample);
  }

  const std::size_t row_bytes = static_cast<std::size_t>(size.width) *
                                ps::core::bytes_per_pixel(buffer.format());

  for (int y = 0; y < size.height; ++y) {
    const std::uint8_t* row = buffer.data() + static_cast<std::size_t>(y) * row_bytes;
    if (TIFFWriteScanline(tif, const_cast<std::uint8_t*>(row), y, 0) < 0) {
      TIFFClose(tif);
      throw std::runtime_error("failed to write TIFF scanline");
    }
  }

  TIFFClose(tif);
}

}  // namespace ps::io
