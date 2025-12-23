#include "ps/tools/tool_manager.h"

#include "ps/tools/brush_tool.h"

namespace ps::tools {

ToolManager& ToolManager::instance() {
  static ToolManager instance;
  return instance;
}

void ToolManager::register_tool(const std::string& id,
                                 std::unique_ptr<Tool> tool) {
  if (!tool) {
    return;
  }

  tools_[id] = std::move(tool);

  if (!active_tool_) {
    active_tool_ = tools_[id].get();
  }
}

Tool* ToolManager::get_tool(const std::string& id) const {
  auto it = tools_.find(id);
  if (it == tools_.end()) {
    return nullptr;
  }
  return it->second.get();
}

void ToolManager::set_active_tool(const std::string& id) {
  Tool* tool = get_tool(id);
  if (tool) {
    active_tool_ = tool;
  }
}

std::vector<std::string> ToolManager::tool_ids() const {
  std::vector<std::string> ids;
  ids.reserve(tools_.size());

  for (const auto& pair : tools_) {
    ids.push_back(pair.first);
  }

  return ids;
}

void ToolManager::register_default_tools() {
  register_tool("brush", std::make_unique<BrushTool>());
}

}  // namespace ps::tools
