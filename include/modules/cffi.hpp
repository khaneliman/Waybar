#pragma once

#include <string>

#include "AModule.hpp"

namespace waybar::modules {

namespace ffi {
extern "C" {
using wbcffi_module = struct wbcffi_module;

using wbcffi_init_info = struct {
  wbcffi_module *obj;
  const char *waybar_version;
  GtkContainer *(*get_root_widget)(wbcffi_module *);
  void (*queue_update)(wbcffi_module *);
};

struct wbcffi_config_entry {
  const char *key;
  const char *value;
};
}
}  // namespace ffi

class CFFI : public AModule {
 public:
  CFFI(const std::string &, const std::string &, const Json::Value &);
  ~CFFI() override;

  auto refresh(int signal) -> void override;
  auto doAction(const std::string &name) -> void override;
  auto update() -> void override;

 private:
  ///
  void *cffi_instance_ = nullptr;

  using InitFn = void *(const ffi::wbcffi_init_info *, const ffi::wbcffi_config_entry *, size_t);
  using DenitFn = void(void *);
  using RefreshFn = void(void *, int);
  using DoActionFn = void(void *, const char *);
  using UpdateFn = void(void *);

  // FFI hooks
  struct {
    std::function<InitFn> init = nullptr;
    std::function<DenitFn> deinit = nullptr;
    std::function<RefreshFn> refresh = [](void *, int) {};
    std::function<DoActionFn> doAction = [](void *, const char *) {};
    std::function<UpdateFn> update = [](void *) {};
  } hooks_;
};

}  // namespace waybar::modules
