#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ps/tools/tool.h"

namespace ps::tools {

class ToolManager {
 public:
  static ToolManager& instance();

  void register_tool(const std::string& id, std::unique_ptr<Tool> tool);

  Tool* get_tool(const std::string& id) const;

  Tool* active_tool() const { return active_tool_; }
  void set_active_tool(const std::string& id);

  std::vector<std::string> tool_ids() const;

  void register_default_tools();

 private:
  ToolManager() = default;
  ~ToolManager() = default;

  ToolManager(const ToolManager&) = delete;
  ToolManager& operator=(const ToolManager&) = delete;

  std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
  Tool* active_tool_ = nullptr;
};

}  // namespace ps::tools
