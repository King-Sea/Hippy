#include "dom/root_node.h"

#include <stack>

#include "dom/diff_utils.h"
#include "dom/render_manager.h"

namespace hippy {
inline namespace dom {

constexpr char kDomCreated[] = "DomCreated";
constexpr char kDomUpdated[] = "DomUpdated";
constexpr char kDomDeleted[] = "DomDeleted";
constexpr char kDomTreeCreated[] = "DomTreeCreated";
constexpr char kDomTreeUpdated[] = "DomTreeUpdated";
constexpr char kDomTreeDeleted[] = "DomTreeDeleted";

RootNode::RootNode(uint32_t id)
        : DomNode(id, 0, 0, "", "",
                  std::unordered_map<std::string, std::shared_ptr<DomValue>>(),
                  std::unordered_map<std::string, std::shared_ptr<DomValue>>()) {
  SetRenderInfo({id, 0, 0});
}

void RootNode::CreateDomNodes(std::vector<std::shared_ptr<DomNode>>&& nodes) {
  std::vector<std::shared_ptr<DomNode>> nodes_to_create;
  for (const auto& node : nodes) {
    std::shared_ptr<DomNode> parent_node = GetNode(node->GetPid());
    if (parent_node == nullptr) {
      continue;
    }
    nodes_to_create.push_back(node);
    node->SetRenderInfo({node->GetId(), node->GetPid(), node->GetIndex()});
    // 解析布局属性
    node->ParseLayoutStyleInfo();
    parent_node->AddChildAt(node, node->GetIndex());

    auto event = std::make_shared<DomEvent>(kDomCreated, node, nullptr);
    node->HandleEvent(event);
    OnDomNodeCreated(node);
  }

  auto event = std::make_shared<DomEvent>(kDomTreeCreated, weak_from_this(), nullptr);
  HandleEvent(event);

  if (!nodes_to_create.empty()) {
    dom_operations_.push_back({DomOperation::kOpCreate, nodes_to_create});
  }
}

void RootNode::UpdateDomNodes(std::vector<std::shared_ptr<DomNode>>&& nodes) {
  std::vector<std::shared_ptr<DomNode>> nodes_to_update;
  for (const auto& it : nodes) {
    std::shared_ptr<DomNode> node = GetNode(it->GetId());
    if (node == nullptr) {
      continue;
    }
    nodes_to_update.push_back(node);
    // diff props
    auto style_diff_value = DiffUtils::DiffProps(*node->GetStyleMap(), *it->GetStyleMap());
    auto ext_diff_value = DiffUtils::DiffProps(*node->GetExtStyle(), *it->GetExtStyle());
    auto style_update = std::get<0>(style_diff_value);
    auto ext_update = std::get<0>(ext_diff_value);
    std::shared_ptr<DomValueMap> diff_value = std::make_shared<DomValueMap>();
    if (style_update) {
      diff_value->insert(style_update->begin(), style_update->end());
    }
    if (ext_update) {
      diff_value->insert(ext_update->begin(), ext_update->end());
    }
    node->SetStyleMap(it->GetStyleMap());
    node->SetExtStyleMap(it->GetExtStyle());
    node->SetDiffStyle(diff_value);
    auto style_delete = std::get<1>(style_diff_value);
    auto ext_delete = std::get<1>(ext_diff_value);
    std::shared_ptr<std::vector<std::string>> delete_value = std::make_shared<std::vector<std::string>>();
    if (style_delete) {
      delete_value->insert(delete_value->end(), style_delete->begin(), style_delete->end());
    }
    if (ext_delete) {
      delete_value->insert(delete_value->end(), ext_delete->begin(), ext_delete->end());
    }
    node->SetDeleteProps(delete_value);
    it->SetDiffStyle(diff_value);
    it->SetDeleteProps(delete_value);
    node->ParseLayoutStyleInfo();
    auto event = std::make_shared<DomEvent>(kDomUpdated, node, nullptr);
    node->HandleEvent(event);
  }

  auto event = std::make_shared<DomEvent>(kDomTreeUpdated, weak_from_this(), nullptr);
  HandleEvent(event);

  if (!nodes_to_update.empty()) {
    dom_operations_.push_back({DomOperation::kOpUpdate, nodes_to_update});
  }
}

void RootNode::DeleteDomNodes(std::vector<std::shared_ptr<DomNode>>&& nodes) {
  std::vector<std::shared_ptr<DomNode>> nodes_to_delete;
  for (const auto & it : nodes) {
    std::shared_ptr<DomNode> node = GetNode(it->GetId());
    if (node == nullptr) {
      continue;
    }
    nodes_to_delete.push_back(node);
    std::shared_ptr<DomNode> parent_node = node->GetParent();
    if (parent_node != nullptr) {
      parent_node->RemoveChildAt(parent_node->IndexOf(node));
    }
    auto event = std::make_shared<DomEvent>(kDomDeleted, node, nullptr);
    node->HandleEvent(event);
    OnDomNodeDeleted(node);
  }

  auto event = std::make_shared<DomEvent>(kDomTreeDeleted, weak_from_this(), nullptr);
  HandleEvent(event);

  if (!nodes_to_delete.empty()) {
    dom_operations_.push_back({DomOperation::kOpDelete, nodes_to_delete});
  }
}

void RootNode::UpdateAnimation(std::vector<std::shared_ptr<DomNode>> &&nodes) {
    std::vector<std::shared_ptr<DomNode>> nodes_to_update;
    for (const auto& it : nodes) {
        std::shared_ptr<DomNode> node = GetNode(it->GetId());
        if (node == nullptr) {
            continue;
        }
        nodes_to_update.push_back(node);
        node->ParseLayoutStyleInfo();
        auto event = std::make_shared<DomEvent>(kDomUpdated, node, nullptr);
        node->HandleEvent(event);
    }
    auto event = std::make_shared<DomEvent>(kDomTreeUpdated, weak_from_this(), nullptr);
    HandleEvent(event);
    if (!nodes_to_update.empty()) {
        dom_operations_.push_back({DomOperation::kOpUpdate, nodes_to_update});
    }
}

void RootNode::CallFunction(uint32_t id, const std::string &name, const DomArgument &param,
                            const CallFunctionCallback &cb) {
  auto node = GetNode(id);
  if (node == nullptr) {
    return;
  }
  node->CallFunction(name, param, cb);
}

void RootNode::SyncWithRenderManager(std::shared_ptr<RenderManager> render_manager) {
  FlushDomOperations(render_manager);
  FlushEventOperations(render_manager);
  DoAndFlushLayout(render_manager);
  render_manager->EndBatch(GetSelf());
}

void RootNode::AddEvent(uint32_t id, const std::string& event_name) {
  event_operations_.push_back({EventOperation::kOpAdd, id, event_name});
}

void RootNode::RemoveEvent(uint32_t id, const std::string& event_name) {
  event_operations_.push_back({EventOperation::kOpRemove, id, event_name});
}

void RootNode::HandleEvent(const std::shared_ptr<DomEvent>& event) {
  auto weak_target = event->GetTarget();
  auto event_name = event->GetType();
  auto target = weak_target.lock();
  if (target) {
    std::stack<std::shared_ptr<DomNode>> capture_list = {};
    // 执行捕获流程，注：target节点event.StopPropagation并不会阻止捕获流程
    if (event->CanCapture()) {
      // 获取捕获列表
      auto parent = target->GetParent();
      while (parent) {
        capture_list.push(parent);
        parent = parent->GetParent();
      }
    }
    auto capture_target_listeners = target->GetEventListener(event_name, true);
    auto bubble_target_listeners = target->GetEventListener(event_name, false);
    // 捕获列表反过来就是冒泡列表，不需要额外遍历生成
    auto runner = delegate_task_runner_.lock();
    if (runner) {
      std::shared_ptr<CommonTask> task = std::make_shared<CommonTask>();
      task->func_ = [capture_list = std::move(capture_list),
              capture_target_listeners = std::move(capture_target_listeners),
              bubble_target_listeners = std::move(bubble_target_listeners),
              dom_event = std::move(event),
              event_name]() mutable {
        // 执行捕获流程
        std::queue<std::shared_ptr<DomNode>> bubble_list = {};
        while (!capture_list.empty()) {
          auto capture_node = capture_list.top();
          capture_list.pop();
          dom_event->SetCurrentTarget(capture_node);  // 设置当前节点，cb里会用到
          auto listeners = capture_node->GetEventListener(event_name, true);
          for (const auto& listener : listeners) {
            listener->cb(dom_event);  // StopPropagation并不会影响同级的回调调用
          }
          if (dom_event->IsPreventCapture()) {  // cb 内部调用了 event.StopPropagation 会阻止捕获
            return;  // 捕获流中StopPropagation不仅会导致捕获流程结束，后面的目标事件和冒泡都会终止
          }
          bubble_list.push(std::move(capture_node));
        }
        // 执行本身节点回调
        dom_event->SetCurrentTarget(dom_event->GetTarget());
        for (const auto& listener : capture_target_listeners) {
          listener->cb(dom_event);
        }
        if (dom_event->IsPreventCapture()) {
          return;
        }
        for (const auto& listener : bubble_target_listeners) {
          listener->cb(dom_event);
        }
        if (dom_event->IsPreventBubble()) {
          return;
        }
        // 执行冒泡流程
        while (!bubble_list.empty()) {
          auto bubble_node = bubble_list.front();
          bubble_list.pop();
          dom_event->SetCurrentTarget(bubble_node);
          auto listeners = bubble_node->GetEventListener(event_name, false);
          for (const auto& listener : listeners) {
            listener->cb(dom_event);
          }
          if (dom_event->IsPreventBubble()) {
            break;
          }
        }
      };
      runner->PostTask(std::move(task));
    }
  }
}

void RootNode::UpdateRenderNode(const std::shared_ptr<DomNode>& node) {
  auto render_manager = render_manager_.lock();
  TDF_BASE_DCHECK(render_manager);
  if (!render_manager) {
    return;
  }
  TDF_BASE_DCHECK(node);

  // 更新 layout tree
  node->ParseLayoutStyleInfo();

  // 更新属性
  std::vector<std::shared_ptr<DomNode>> nodes;
  nodes.push_back(node);
  render_manager->UpdateRenderNode(GetSelf(), std::move(nodes));

  SyncWithRenderManager(render_manager);
}

std::shared_ptr<DomNode> RootNode::GetNode(uint32_t id) {
  if (id == GetId()) {
    return shared_from_this();
  }
  auto found = nodes_.find(id);
  if (found == nodes_.end()) {
    return nullptr;
  }
  return found->second.lock();
}

std::tuple<float, float> RootNode::GetRootSize() {
  return GetLayoutSize();
}

void RootNode::SetRootSize(float width, float height) {
  SetLayoutSize(width, height);
}

void RootNode::SetRenderManager(std::shared_ptr<RenderManager> render_manager) {
  SetRootNode(GetSelf());
  render_manager_ = render_manager;
}

void RootNode::DoAndFlushLayout(std::shared_ptr<RenderManager> render_manager) {
  // Before Layout
  render_manager->BeforeLayout(GetSelf());
  // 触发布局计算
  std::vector<std::shared_ptr<DomNode>> layout_changed_nodes;
  DoLayout(layout_changed_nodes);
  // After Layout
  render_manager->AfterLayout(GetSelf());

  if (!layout_changed_nodes.empty()) {
    render_manager->UpdateLayout(GetSelf(), layout_changed_nodes);
  }
}

void RootNode::FlushDomOperations(std::shared_ptr<RenderManager> render_manager) {
  for (auto& dom_operation : dom_operations_) {
    switch (dom_operation.op) {
      case DomOperation::kOpCreate:
        render_manager->CreateRenderNode(GetSelf(), std::move(dom_operation.nodes));
        break;
      case DomOperation::kOpUpdate:
        render_manager->UpdateRenderNode(GetSelf(), std::move(dom_operation.nodes));
        break;
      case DomOperation::kOpDelete:
        render_manager->DeleteRenderNode(GetSelf(), std::move(dom_operation.nodes));
        break;
      default:
        break;
    }
  }
  dom_operations_.clear();
}

void RootNode::FlushEventOperations(std::shared_ptr<RenderManager> render_manager) {
  for (auto& event_operation : event_operations_) {
    const auto& node = GetNode(event_operation.id);
    if (node == nullptr) {
      continue;
    }

    switch (event_operation.op) {
      case EventOperation::kOpAdd:
        render_manager->AddEventListener(GetSelf(), node, event_operation.name);
        break;
      case EventOperation::kOpRemove:
        render_manager->RemoveEventListener(GetSelf(), node, event_operation.name);
        break;
      default:
        break;
    }
  }
  event_operations_.clear();
}

void RootNode::OnDomNodeCreated(const std::shared_ptr<DomNode>& node) {
  nodes_.insert(std::make_pair(node->GetId(), node));
}

void RootNode::OnDomNodeDeleted(const std::shared_ptr<DomNode> &node) {
  if (node) {
    for (const auto &child : node->GetChildren()) {
      if (child) {
        OnDomNodeDeleted(child);
      }
    }
    nodes_.erase(node->GetId());
  }
}

std::shared_ptr<RootNode> RootNode::GetSelf() {
  return std::static_pointer_cast<RootNode>(shared_from_this());
}

}  // namespace dom
}  // namespace hippy
