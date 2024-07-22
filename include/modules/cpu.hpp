#pragma once

#include <fmt/format.h>

#include <string>
#include <vector>

#include "ALabel.hpp"
#include "util/sleeper_thread.hpp"

namespace waybar::modules {

class Cpu : public ALabel {
 public:
  Cpu(const std::string&, const Json::Value&);
  ~Cpu() override = default;
  auto update() -> void override;

 private:
  std::vector<std::tuple<size_t, size_t>> prev_times_;

  util::SleeperThread thread_;
};

}  // namespace waybar::modules
