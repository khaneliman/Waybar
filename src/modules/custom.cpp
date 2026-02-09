#include "modules/custom.hpp"

#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>

waybar::modules::Custom::Custom(const std::string& name, const std::string& id,
                                const Json::Value& config, const std::string& output_name)
    : ALabel(config, "custom-" + name, id, "{}"),
      name_(name),
      output_name_(output_name),
      id_(id),
      tooltip_format_enabled_{config_["tooltip-format"].isString()},
      percentage_(0),
      fp_(nullptr),
      pid_(-1) {
  if (config.isNull()) {
    spdlog::warn("There is no configuration for 'custom/{}', element will be hidden", name);
  }
  dp.emit();
  if (!config_["signal"].empty() && config_["interval"].empty() &&
      config_["restart-interval"].empty()) {
    waitingWorker();
  } else if (interval_.count() > 0) {
    delayWorker();
  } else if (config_["exec"].isString()) {
    continuousWorker();
  }
}

waybar::modules::Custom::~Custom() {
  if (pid_ != -1) {
    killpg(pid_, SIGTERM);
    waitpid(pid_, NULL, 0);
    pid_ = -1;
  }
}

void waybar::modules::Custom::delayWorker() {
  thread_ = [this] {
    for (auto it = this->pid_children_.begin(); it != this->pid_children_.end();) {
      int status = 0;
      const auto pid = static_cast<pid_t>(*it);
      const auto waited = waitpid(pid, &status, WNOHANG);
      if (waited == 0) {
        ++it;
        continue;
      }
      if (waited == -1 && errno != ECHILD) {
        ++it;
        continue;
      }
      it = this->pid_children_.erase(it);
    }

    bool can_update = true;
    if (config_["exec-if"].isString()) {
      output_ = util::command::execNoRead(config_["exec-if"].asString());
      if (output_.exit_code != 0) {
        can_update = false;
        dp.emit();
      }
    }
    if (can_update) {
      if (config_["exec"].isString()) {
        output_ = util::command::exec(config_["exec"].asString(), output_name_);
      }
      dp.emit();
    }
    thread_.sleep_for(interval_);
  };
}

void waybar::modules::Custom::continuousWorker() {
  auto cmd = config_["exec"].asString();
  pid_ = -1;
  fp_ = util::command::open(cmd, pid_, output_name_);
  if (!fp_) {
    throw std::runtime_error("Unable to open " + cmd);
  }
  thread_ = [this, cmd] {
    auto handle_stopped_process = [this, &cmd](int default_exit_code) {
      int exit_code = default_exit_code;
      if (fp_) {
        exit_code = WEXITSTATUS(util::command::close(fp_, pid_));
        fp_ = nullptr;
        continuous_buffer_.clear();
      }
      if (exit_code != 0) {
        output_ = {exit_code, ""};
        dp.emit();
        spdlog::error("{} stopped unexpectedly, is it endless?", name_);
      }
      if (config_["restart-interval"].isNumeric()) {
        pid_ = -1;
        thread_.sleep_for(std::chrono::milliseconds(
            std::max(1L,  // Minimum 1ms due to millisecond precision
                     static_cast<long>(config_["restart-interval"].asDouble() * 1000))));
        fp_ = util::command::open(cmd, pid_, output_name_);
        if (!fp_) {
          throw std::runtime_error("Unable to open " + cmd);
        }
        continuous_buffer_.clear();
      } else {
        thread_.stop();
      }
    };

    if (fp_ == nullptr) {
      return;
    }

    auto fd = fileno(fp_);
    if (fd == -1) {
      handle_stopped_process(1);
      return;
    }

    struct pollfd poll_fd{
        .fd = fd,
        .events = POLLIN | POLLHUP | POLLERR,
        .revents = 0,
    };

    auto poll_rc = poll(&poll_fd, 1, 250);
    if (poll_rc == 0) {
      return;
    }
    if (poll_rc == -1) {
      if (errno == EINTR) {
        return;
      }
      handle_stopped_process(1);
      return;
    }
    if ((poll_fd.revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
      return;
    }

    std::array<char, 4096> chunk = {};
    auto bytes_read = read(fd, chunk.data(), chunk.size());
    if (bytes_read > 0) {
      continuous_buffer_.append(chunk.data(), static_cast<size_t>(bytes_read));
      for (auto new_line_pos = continuous_buffer_.find('\n'); new_line_pos != std::string::npos;
           new_line_pos = continuous_buffer_.find('\n')) {
        std::string output = continuous_buffer_.substr(0, new_line_pos);
        continuous_buffer_.erase(0, new_line_pos + 1);
        if (!output.empty() && output.back() == '\r') {
          output.pop_back();
        }
        output_ = {0, output};
        dp.emit();
      }
      return;
    }

    if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      return;
    }

    handle_stopped_process(1);
  };
}

void waybar::modules::Custom::waitingWorker() {
  thread_ = [this] {
    bool can_update = true;
    if (config_["exec-if"].isString()) {
      output_ = util::command::execNoRead(config_["exec-if"].asString());
      if (output_.exit_code != 0) {
        can_update = false;
        dp.emit();
      }
    }
    if (can_update) {
      if (config_["exec"].isString()) {
        output_ = util::command::exec(config_["exec"].asString(), output_name_);
      }
      dp.emit();
    }
    thread_.sleep();
  };
}

void waybar::modules::Custom::refresh(int sig) {
  if (config_["signal"].isInt() && sig == SIGRTMIN + config_["signal"].asInt()) {
    thread_.wake_up();
  }
}

void waybar::modules::Custom::handleEvent() {
  if (!config_["exec-on-event"].isBool() || config_["exec-on-event"].asBool()) {
    thread_.wake_up();
  }
}

bool waybar::modules::Custom::handleScroll(GdkEventScroll* e) {
  auto ret = ALabel::handleScroll(e);
  handleEvent();
  return ret;
}

bool waybar::modules::Custom::handleToggle(GdkEventButton* const& e) {
  auto ret = ALabel::handleToggle(e);
  handleEvent();
  return ret;
}

auto waybar::modules::Custom::update() -> void {
  // Hide label if output is empty
  if ((config_["exec"].isString() || config_["exec-if"].isString()) &&
      (output_.out.empty() || output_.exit_code != 0)) {
    event_box_.hide();
  } else {
    if (config_["return-type"].asString() == "json") {
      parseOutputJson();
    } else {
      parseOutputRaw();
    }

    try {
      auto str = fmt::format(fmt::runtime(format_), fmt::arg("text", text_), fmt::arg("alt", alt_),
                             fmt::arg("icon", getIcon(percentage_, alt_)),
                             fmt::arg("percentage", percentage_));
      if ((config_["hide-empty-text"].asBool() && text_.empty()) || str.empty()) {
        event_box_.hide();
      } else {
        label_.set_markup(str);
        if (tooltipEnabled()) {
          if (tooltip_format_enabled_) {
            auto tooltip = config_["tooltip-format"].asString();
            tooltip = fmt::format(
                fmt::runtime(tooltip), fmt::arg("text", text_), fmt::arg("alt", alt_),
                fmt::arg("icon", getIcon(percentage_, alt_)), fmt::arg("percentage", percentage_));
            label_.set_tooltip_markup(tooltip);
          } else if (text_ == tooltip_) {
            label_.set_tooltip_markup(str);
          } else {
            label_.set_tooltip_markup(tooltip_);
          }
        }
        auto style = label_.get_style_context();
        auto classes = style->list_classes();
        for (auto const& c : classes) {
          if (c == id_) continue;
          style->remove_class(c);
        }
        for (auto const& c : class_) {
          style->add_class(c);
        }
        style->add_class("flat");
        style->add_class("text-button");
        style->add_class(MODULE_CLASS);
        event_box_.show();
      }
    } catch (const fmt::format_error& e) {
      if (std::strcmp(e.what(), "cannot switch from manual to automatic argument indexing") != 0)
        throw;

      throw fmt::format_error(
          "mixing manual and automatic argument indexing is no longer supported; "
          "try replacing \"{}\" with \"{text}\" in your format specifier");
    }
  }
  // Call parent update
  ALabel::update();
}

void waybar::modules::Custom::parseOutputRaw() {
  std::istringstream output(output_.out);
  std::string line;
  int i = 0;
  while (getline(output, line)) {
    Glib::ustring validated_line = line;
    if (!validated_line.validate()) {
      validated_line = validated_line.make_valid();
    }

    if (i == 0) {
      if (config_["escape"].isBool() && config_["escape"].asBool()) {
        text_ = Glib::Markup::escape_text(validated_line);
        tooltip_ = Glib::Markup::escape_text(validated_line);
      } else {
        text_ = validated_line;
        tooltip_ = validated_line;
      }
      tooltip_ = validated_line;
      class_.clear();
    } else if (i == 1) {
      if (config_["escape"].isBool() && config_["escape"].asBool()) {
        tooltip_ = Glib::Markup::escape_text(validated_line);
      } else {
        tooltip_ = validated_line;
      }
    } else if (i == 2) {
      class_.push_back(validated_line);
    } else {
      break;
    }
    i++;
  }
}

void waybar::modules::Custom::parseOutputJson() {
  std::istringstream output(output_.out);
  std::string line;
  class_.clear();
  while (getline(output, line)) {
    auto parsed = parser_.parse(line);
    if (config_["escape"].isBool() && config_["escape"].asBool()) {
      text_ = Glib::Markup::escape_text(parsed["text"].asString());
    } else {
      text_ = parsed["text"].asString();
    }
    if (config_["escape"].isBool() && config_["escape"].asBool()) {
      alt_ = Glib::Markup::escape_text(parsed["alt"].asString());
    } else {
      alt_ = parsed["alt"].asString();
    }
    if (config_["escape"].isBool() && config_["escape"].asBool()) {
      tooltip_ = Glib::Markup::escape_text(parsed["tooltip"].asString());
    } else {
      tooltip_ = parsed["tooltip"].asString();
    }
    if (parsed["class"].isString()) {
      class_.push_back(parsed["class"].asString());
    } else if (parsed["class"].isArray()) {
      for (auto const& c : parsed["class"]) {
        class_.push_back(c.asString());
      }
    }
    if (!parsed["percentage"].asString().empty() && parsed["percentage"].isNumeric()) {
      percentage_ = (int)lround(parsed["percentage"].asFloat());
    } else {
      percentage_ = 0;
    }
    break;
  }
}
