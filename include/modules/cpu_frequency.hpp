#pragma once

#include <fmt/format.h>

#include <string>
#include <vector>

#include "ALabel.hpp"
#include "util/sleeper_thread.hpp"

namespace waybar::modules {

class CpuFrequency : public ALabel {
 public:
  CpuFrequency(const std::string&, const Json::Value&);
  ~CpuFrequency() override = default;
  auto update() -> void override;

  // This is a static member because it is also used by the cpu module.
  static std::tuple<float, float, float> getCpuFrequency();

 private:
  static std::vector<float> parseCpuFrequencies();

  util::SleeperThread thread_;
};

}  // namespace waybar::modules
