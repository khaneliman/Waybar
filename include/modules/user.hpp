#pragma once

#include <fmt/chrono.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>

#include "AIconLabel.hpp"
#include "util/sleeper_thread.hpp"

#if HAVE_CPU_LINUX
#include <sys/sysinfo.h>
#endif

namespace waybar::modules {
class User : public AIconLabel {
 public:
  User(const std::string&, const Json::Value&);
  ~User() override = default;
  auto update() -> void override;

  bool handleToggle(GdkEventButton* const& e) override;

 private:
  util::SleeperThread thread_;

  static constexpr inline int defaultUserImageWidth_ = 20;
  static constexpr inline int defaultUserImageHeight_ = 20;
  static std::string get_user_login() { return Glib::get_user_name(); }
  static std::string get_user_home_dir() { return Glib::get_home_dir(); }
  static std::string get_default_user_avatar_path() {
    return waybar::modules::User::get_user_home_dir() + "/" + ".face";
  }

  static long uptime_as_seconds() {
    long uptime = 0;

#if HAVE_CPU_LINUX
    struct sysinfo s_info;
    if (0 == sysinfo(&s_info)) {
      uptime = s_info.uptime;
    }
#endif

#if HAVE_CPU_BSD
    struct timespec s_info;
    if (0 == clock_gettime(CLOCK_UPTIME_PRECISE, &s_info)) {
      uptime = s_info.tv_sec;
    }
#endif

    return uptime;
  }

  void init_default_user_avatar(int width, int height);
  void init_user_avatar(const std::string& path, int width, int height);
  void init_avatar(const Json::Value& config);
  void init_update_worker();
};
}  // namespace waybar::modules
