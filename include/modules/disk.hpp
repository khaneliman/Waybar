#pragma once

#include <fmt/format.h>
#include <sys/statvfs.h>

#include "ALabel.hpp"
#include "util/sleeper_thread.hpp"

namespace waybar::modules {

class Disk : public ALabel {
 public:
  Disk(const std::string&, const Json::Value&);
  ~Disk() override = default;
  auto update() -> void override;

 private:
  util::SleeperThread thread_;
  std::string path_;
  std::string unit_;

  static float calc_specific_divisor(const std::string& divisor);
};

}  // namespace waybar::modules
