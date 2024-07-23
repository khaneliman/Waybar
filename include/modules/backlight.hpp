#pragma once

#include <string>

#include "ALabel.hpp"
#include "util/backlight_backend.hpp"

struct udev;
struct udev_device;

namespace waybar::modules {

class Backlight : public ALabel {
 public:
  Backlight(const std::string &, const Json::Value &);
  ~Backlight() override = default;
  auto update() -> void override;

  bool handleScroll(GdkEventScroll *e) override;

  const std::string preferred_device_;

  std::string previous_format_;

  util::BacklightBackend backend;
};
}  // namespace waybar::modules
