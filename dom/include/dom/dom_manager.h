#pragma once

#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "core/base/common.h"
#include "core/base/task_runner.h"
#include "core/task/common_task.h"
#include "dom/dom_action_interceptor.h"
#include "dom/dom_argument.h"
#include "dom/dom_event.h"
#include "dom/dom_listener.h"
#include "dom/dom_value.h"
#include "dom/layout_node.h"
#include "dom/scene.h"

namespace hippy {
inline namespace dom {

class DomNode;
class RenderManager;
class RootNode;
class LayerOptimizedRenderManager;

// This class is used to mainpulate dom. Please note that the member
// function of this class must be run in dom thread. If you want to call
// in other thread please use PostTask.
// Example:
//    std::vector<std::function<void()>> ops;
//    ops.emplace_back([]() {
//      some_ops();
//    });
//    dom_manager->PostTask(Scene(std::move(ops)));
class DomManager : public std::enable_shared_from_this<DomManager> {
 public:
  using DomValue = tdf::base::DomValue;
  using TaskRunner = hippy::base::TaskRunner;

  DomManager(uint32_t root_id);
  ~DomManager() = default;

  int32_t GetId() { return id_; }

  inline std::shared_ptr<RenderManager> GetRenderManager() { return render_manager_.lock(); }
  void SetRenderManager(std::shared_ptr<RenderManager> render_manager);
  inline void SetDelegateTaskRunner(std::shared_ptr<TaskRunner> runner) { delegate_task_runner_ = runner; }
  uint32_t GetRootId() const;
  std::shared_ptr<DomNode> GetNode(std::weak_ptr<RootNode> root_node, uint32_t id) const;

  void CreateDomNodes(std::weak_ptr<RootNode> root_node,
                      std::vector<std::shared_ptr<DomNode>> &&nodes);

  void UpdateDomNodes(std::weak_ptr<RootNode> root_node,
                      std::vector<std::shared_ptr<DomNode>> &&nodes);

  void DeleteDomNodes(std::weak_ptr<RootNode> root_node,
                      std::vector<std::shared_ptr<DomNode>> &&nodes);

  void UpdateAnimation(std::weak_ptr<RootNode> root_node,
                       std::vector<std::shared_ptr<DomNode>> &&nodes);

  void EndBatch(std::weak_ptr<RootNode> root_node);

  // 返回0代表失败，正常id从1开始
  void AddEventListener(std::weak_ptr<RootNode> root_node, uint32_t id, const std::string &event_name, uint64_t listener_id,
                        bool use_capture, const EventCallback &cb);

  void RemoveEventListener(std::weak_ptr<RootNode> root_node, uint32_t id, const std::string &name,
                           uint64_t listener_id);

  void CallFunction(std::weak_ptr<RootNode> root_node, uint32_t id, const std::string &name,
                    const DomArgument &param, const CallFunctionCallback &cb);

  void SetRootSize(std::weak_ptr<RootNode> root_node, float width, float height);
  void DoLayout(std::weak_ptr<RootNode> root_node);
  void PostTask(const Scene&& scene);
  void StartTaskRunner() { dom_task_runner_->Start(); }
  void TerminateTaskRunner() { dom_task_runner_->Terminate(); }
  static void Insert(const std::shared_ptr<DomManager>& dom_manager);
  static std::shared_ptr<DomManager> Find(int32_t id);
  static bool Erase(int32_t id);
  static bool Erase(const std::shared_ptr<DomManager>& dom_manager);
  void AddInterceptor(std::shared_ptr<DomActionInterceptor> interceptor);

 private:
  int32_t id_;
  uint32_t root_id_;
  std::shared_ptr<LayerOptimizedRenderManager> optimized_render_manager_;
  std::weak_ptr<RenderManager> render_manager_;
  std::weak_ptr<TaskRunner> delegate_task_runner_;
  std::shared_ptr<TaskRunner> dom_task_runner_;
  std::vector<std::shared_ptr<DomActionInterceptor>> interceptors_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(DomManager);

  friend DomNode;
};

}  // namespace dom
}  // namespace hippy
