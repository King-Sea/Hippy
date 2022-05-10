#define ENABLE_LAYER_OPTIMIZATION

#include "dom/dom_manager.h"

#include <mutex>
#include <stack>
#include <utility>

#include "dom/animation_manager.h"
#include "dom/diff_utils.h"
#include "dom/dom_action_interceptor.h"
#include "dom/dom_event.h"
#include "dom/dom_node.h"
#include "dom/layer_optimized_render_manager.h"
#include "dom/macro.h"
#include "dom/render_manager.h"
#include "dom/root_node.h"

#define DCHECK_RUN_THREAD() \
  { TDF_BASE_DCHECK(dom_task_runner_->Id() == hippy::base::ThreadId::GetCurrent()); }

namespace hippy {
inline namespace dom {

using DomNode = hippy::DomNode;

static std::unordered_map<int32_t, std::shared_ptr<DomManager>> dom_manager_map;
static std::mutex mutex;
static std::atomic<int32_t> global_dom_manager_key{0};

constexpr uint32_t kInvalidListenerId = 0;

DomManager::DomManager(uint32_t root_id) {
  root_id_ = root_id;
  dom_task_runner_ = std::make_shared<hippy::base::TaskRunner>();
  id_ = global_dom_manager_key.fetch_add(1);
}

void DomManager::Insert(const std::shared_ptr<DomManager>& dom_manager) {
  std::lock_guard<std::mutex> lock(mutex);
  dom_manager_map[dom_manager->id_] = dom_manager;
}

std::shared_ptr<DomManager> DomManager::Find(int32_t id) {
  std::lock_guard<std::mutex> lock(mutex);
  const auto it = dom_manager_map.find(id);
  if (it == dom_manager_map.end()) {
    return nullptr;
  }
  return it->second;
}

bool DomManager::Erase(int32_t id) {
  std::lock_guard<std::mutex> lock(mutex);
  const auto it = dom_manager_map.find(id);
  if (it == dom_manager_map.end()) {
    return false;
  }
  dom_manager_map.erase(it);
  return true;
}

bool DomManager::Erase(const std::shared_ptr<DomManager>& dom_manager) { return DomManager::Erase(dom_manager->id_); }

void DomManager::SetRenderManager(std::shared_ptr<RenderManager> render_manager) {
#ifdef ENABLE_LAYER_OPTIMIZATION
  optimized_render_manager_ = std::make_shared<LayerOptimizedRenderManager>(render_manager);
  render_manager_ = optimized_render_manager_;
#else
  render_manager_ = render_manager;
#endif
}

uint32_t DomManager::GetRootId() const { return root_id_; }

std::shared_ptr<DomNode> DomManager::GetNode(std::weak_ptr<RootNode> root_node, uint32_t id) const {
  auto host = root_node.lock();
  if (host == nullptr) {
    return nullptr;
  }
  return host->GetNode(id);
}

void DomManager::CreateDomNodes(std::weak_ptr<RootNode> root_node,
                                std::vector<std::shared_ptr<DomNode>>&& nodes) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  for (std::shared_ptr<DomActionInterceptor> interceptor : interceptors_) {
    interceptor->OnDomNodeCreate(nodes);
  }
  host->CreateDomNodes(std::move(nodes));
}

void DomManager::UpdateDomNodes(std::weak_ptr<RootNode> root_node,
                                std::vector<std::shared_ptr<DomNode>>&& nodes) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  for (std::shared_ptr<DomActionInterceptor> interceptor : interceptors_) {
    interceptor->OnDomNodeUpdate(nodes);
  }
  host->UpdateDomNodes(std::move(nodes));
}

void DomManager::UpdateAnimation(std::weak_ptr<RootNode> root_node,
                                 std::vector<std::shared_ptr<DomNode>>&& nodes) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  host->UpdateAnimation(std::move(nodes));
}

void DomManager::DeleteDomNodes(std::weak_ptr<RootNode> root_node,
                                std::vector<std::shared_ptr<DomNode>>&& nodes) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  for (std::shared_ptr<DomActionInterceptor> interceptor : interceptors_) {
    interceptor->OnDomNodeDelete(nodes);
  }
  host->DeleteDomNodes(std::move(nodes));
}

void DomManager::EndBatch(std::weak_ptr<RootNode> root_node) {
  DCHECK_RUN_THREAD()
  auto render_manager = render_manager_.lock();
  TDF_BASE_DCHECK(render_manager);
  if (!render_manager) {
    return;
  }
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  host->SyncWithRenderManager(render_manager);
}

void DomManager::AddEventListener(std::weak_ptr<RootNode> root_node,
                                  uint32_t id, const std::string& name, bool use_capture, const EventCallback& cb,
                                  const CallFunctionCallback& callback) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  auto node = host->GetNode(id);
  if (!node && callback) {
    callback(std::make_shared<DomArgument>(DomValue(kInvalidListenerId)));
    return;
  }
  node->AddEventListener(name, use_capture, cb, callback);
}

void DomManager::RemoveEventListener(std::weak_ptr<RootNode> root_node,
                                     uint32_t id, const std::string& name, uint32_t listener_id) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  auto node = host->GetNode(id);
  if (!node) {
    return;
  }
  node->RemoveEventListener(name, listener_id);
}

void DomManager::CallFunction(std::weak_ptr<RootNode> root_node,
                              uint32_t id, const std::string& name, const DomArgument& param,
                              const CallFunctionCallback& cb) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  host->CallFunction(id, name, param, cb);
}

void DomManager::SetRootSize(std::weak_ptr<RootNode> root_node, float width, float height) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  host->SetRootSize(width, height);
}

void DomManager::DoLayout(std::weak_ptr<RootNode> root_node) {
  DCHECK_RUN_THREAD()
  auto host = root_node.lock();
  if (host == nullptr) {
    return;
  }
  auto render_manager = render_manager_.lock();
  // check render_manager, measure text dependent render_manager
  TDF_BASE_DCHECK(render_manager);
  if (!render_manager) {
    return;
  }
  host->DoAndFlushLayout(render_manager);
}

void DomManager::PostTask(const Scene&& scene) {
  std::shared_ptr<CommonTask> task = std::make_shared<CommonTask>();
  task->func_ = [scene = std::move(scene)] { scene.Build(); };
  dom_task_runner_->PostTask(std::move(task));
}

void DomManager::AddInterceptor(std::shared_ptr<DomActionInterceptor> interceptor) {
  interceptors_.push_back(interceptor);
}

}  // namespace dom
}  // namespace hippy
