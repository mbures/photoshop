#include "ps/core/command.h"

#include <algorithm>
#include <cstring>

namespace ps::core {

ImageCommand::ImageCommand(ImageDocument& doc) : document_(doc) {}

void ImageCommand::save_channel(std::size_t index) {
  if (index >= document_.channels().size()) {
    return;
  }

  const auto& channel = document_.channel_at(index);
  ChannelBackup backup;
  backup.index = index;
  backup.buffer = ImageBuffer(channel.buffer.size(), channel.buffer.format());

  std::memcpy(backup.buffer.data(), channel.buffer.data(),
              channel.buffer.byte_size());

  saved_channels_.push_back(std::move(backup));
}

void ImageCommand::save_all_channels() {
  saved_channels_.clear();
  for (std::size_t i = 0; i < document_.channels().size(); ++i) {
    save_channel(i);
  }
}

void ImageCommand::restore_channels() {
  for (auto& backup : saved_channels_) {
    if (backup.index >= document_.channels().size()) {
      continue;
    }

    auto& channel = document_.channel_at(backup.index);
    std::memcpy(channel.buffer.data(), backup.buffer.data(),
                backup.buffer.byte_size());
  }
}

void ImageCommand::undo() { restore_channels(); }

FillCommand::FillCommand(ImageDocument& doc, std::size_t channel_index,
                         std::uint8_t value)
    : ImageCommand(doc),
      channel_index_(channel_index),
      fill_value_(value) {}

void FillCommand::execute() {
  save_channel(channel_index_);

  auto& channel = document_.channel_at(channel_index_);
  std::memset(channel.buffer.data(), fill_value_, channel.buffer.byte_size());
}

ClearCommand::ClearCommand(ImageDocument& doc, std::size_t channel_index)
    : ImageCommand(doc), channel_index_(channel_index) {}

void ClearCommand::execute() {
  save_channel(channel_index_);

  auto& channel = document_.channel_at(channel_index_);
  std::memset(channel.buffer.data(), 0, channel.buffer.byte_size());
}

}  // namespace ps::core
