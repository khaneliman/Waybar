#include "modules/user.hpp"

#include <fmt/chrono.h>
#include <glibmm/miscutils.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>

#include "glibmm/fileutils.h"

#if HAVE_CPU_BSD
#include <time.h>
#endif

const static int LEFT_MOUSE_BUTTON_CODE = 1;

namespace waybar::modules {
User::User(const std::string& id, const Json::Value& config)
    : AIconLabel(config, "user", id, "{user} {work_H}:{work_M}", 60, false, true, true) {
  AIconLabel::box_.set_spacing(0);
  if (AIconLabel::iconEnabled()) {
    this->init_avatar(AIconLabel::config_);
  }
  this->init_update_worker();
}

bool User::handleToggle(GdkEventButton* const& e) {
  if (AIconLabel::config_["open-on-click"].isBool() &&
      AIconLabel::config_["open-on-click"].asBool() && e->button == LEFT_MOUSE_BUTTON_CODE) {
    std::string openPath = waybar::modules::User::get_user_home_dir();
    if (AIconLabel::config_["open-path"].isString()) {
      std::string customPath = AIconLabel::config_["open-path"].asString();
      if (!customPath.empty()) {
        openPath = std::move(customPath);
      }
    }

    Gio::AppInfo::launch_default_for_uri("file:///" + openPath);
  }
  return true;
}

void User::init_update_worker() {
  this->thread_ = [this] {
    ALabel::dp.emit();
    auto now = std::chrono::system_clock::now();
    auto diff = now.time_since_epoch() % ALabel::interval_;
    this->thread_.sleep_for(ALabel::interval_ - diff);
  };
}

void User::init_avatar(const Json::Value& config) {
  int height = config["height"].isUInt() ? config["height"].asUInt()
                                         : waybar::modules::User::defaultUserImageHeight_;
  int width = config["width"].isUInt() ? config["width"].asUInt()
                                       : waybar::modules::User::defaultUserImageWidth_;

  if (config["avatar"].isString()) {
    std::string userAvatar = config["avatar"].asString();
    if (!userAvatar.empty()) {
      this->init_user_avatar(userAvatar, width, height);
      return;
    }
  }

  this->init_default_user_avatar(width, width);
}

void User::init_default_user_avatar(int width, int height) {
  this->init_user_avatar(waybar::modules::User::get_default_user_avatar_path(), width, height);
}

void User::init_user_avatar(const std::string& path, int width, int height) {
  if (Glib::file_test(path, Glib::FILE_TEST_EXISTS)) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf_ = Gdk::Pixbuf::create_from_file(path, width, height);
    AIconLabel::image_.set(pixbuf_);
  } else {
    AIconLabel::box_.remove(AIconLabel::image_);
  }
}

auto User::update() -> void {
  std::string systemUser = waybar::modules::User::get_user_login();
  std::transform(systemUser.cbegin(), systemUser.cend(), systemUser.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  long uptimeSeconds = waybar::modules::User::uptime_as_seconds();
  auto workSystemTimeSeconds = std::chrono::seconds(uptimeSeconds);
  auto currentSystemTime = std::chrono::system_clock::now();
  auto startSystemTime = currentSystemTime - workSystemTimeSeconds;
  long workSystemDays = uptimeSeconds / 86400;

  auto label = fmt::format(
      fmt::runtime(ALabel::format_), fmt::arg("up_H", fmt::format("{:%H}", startSystemTime)),
      fmt::arg("up_M", fmt::format("{:%M}", startSystemTime)),
      fmt::arg("up_d", fmt::format("{:%d}", startSystemTime)),
      fmt::arg("up_m", fmt::format("{:%m}", startSystemTime)),
      fmt::arg("up_Y", fmt::format("{:%Y}", startSystemTime)), fmt::arg("work_d", workSystemDays),
      fmt::arg("work_H", fmt::format("{:%H}", workSystemTimeSeconds)),
      fmt::arg("work_M", fmt::format("{:%M}", workSystemTimeSeconds)),
      fmt::arg("work_S", fmt::format("{:%S}", workSystemTimeSeconds)),
      fmt::arg("user", systemUser));
  ALabel::label_.set_markup(label);
  AIconLabel::update();
}
};  // namespace waybar::modules
