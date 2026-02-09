#include "modules/sway/ipc/client.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>

namespace waybar::modules::sway {

Ipc::Ipc() {
  const std::string socketPath = getSocketPath();
  fd_ = open(socketPath);
  try {
    fd_event_ = open(socketPath);
  } catch (...) {
    ::close(fd_);
    fd_ = -1;
    throw;
  }
}

Ipc::~Ipc() {
  thread_.stop();

  if (fd_ >= 0) {
    // To fail the IPC header
    if (write(fd_, "close-sway-ipc", 14) == -1) {
      spdlog::error("Failed to close sway IPC");
    }
    ::close(fd_);
    fd_ = -1;
  }
  if (fd_event_ >= 0) {
    if (write(fd_event_, "close-sway-ipc", 14) == -1) {
      spdlog::error("Failed to close sway IPC event handler");
    }
    ::close(fd_event_);
    fd_event_ = -1;
  }
}

void Ipc::setWorker(std::function<void()>&& func) { thread_ = func; }

std::string Ipc::parseSocketPathOutput(const std::string& output) {
  std::string path = output;
  while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) {
    path.pop_back();
  }

  if (path.empty()) {
    throw std::runtime_error("Socket path is empty");
  }
  return path;
}

std::string Ipc::readSocketPathFromStream(FILE* in) {
  std::string str_buf;
  char buf[512] = {0};
  while (fgets(buf, sizeof(buf), in) != nullptr) {
    str_buf.append(buf);
  }
  return parseSocketPathOutput(str_buf);
}

const std::string Ipc::getSocketPath() const {
  const char* env = getenv("SWAYSOCK");
  if (env != nullptr) {
    return std::string(env);
  }
  std::unique_ptr<FILE, int (*)(FILE*)> in(popen("sway --get-socketpath 2>/dev/null", "r"), pclose);
  if (in == nullptr) {
    throw std::runtime_error("Failed to get socket path");
  }

  return readSocketPathFromStream(in.get());
}

int Ipc::open(const std::string& socketPath) const {
  int32_t fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    throw std::runtime_error("Unable to open Unix socket");
  }
  (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
  addr.sun_path[sizeof(addr.sun_path) - 1] = 0;
  const auto socket_len = static_cast<socklen_t>(sizeof(struct sockaddr_un));
  if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), socket_len) == -1) {
    ::close(fd);
    throw std::runtime_error("Unable to connect to Sway");
  }
  return fd;
}

struct Ipc::ipc_response Ipc::recv(int fd) {
  std::string header;
  header.resize(ipc_header_size_);
  auto data32 = reinterpret_cast<uint32_t*>(header.data() + ipc_magic_.size());
  size_t total = 0;

  while (total < ipc_header_size_) {
    auto res = ::recv(fd, header.data() + total, ipc_header_size_ - total, 0);
    if (fd_event_ == -1 || fd_ == -1) {
      // IPC is closed so just return an empty response
      return {0, 0, ""};
    }
    if (res <= 0) {
      throw std::runtime_error("Unable to receive IPC header");
    }
    total += res;
  }
  auto magic = std::string(header.data(), header.data() + ipc_magic_.size());
  if (magic != ipc_magic_) {
    throw std::runtime_error("Invalid IPC magic");
  }

  total = 0;
  std::string payload;
  payload.resize(data32[0]);
  while (total < data32[0]) {
    auto res = ::recv(fd, payload.data() + total, data32[0] - total, 0);
    if (res < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      throw std::runtime_error("Unable to receive IPC payload");
    }
    total += res;
  }
  return {data32[0], data32[1], &payload.front()};
}

struct Ipc::ipc_response Ipc::send(int fd, uint32_t type, const std::string& payload) {
  std::string header;
  header.resize(ipc_header_size_);
  auto data32 = reinterpret_cast<uint32_t*>(header.data() + ipc_magic_.size());
  memcpy(header.data(), ipc_magic_.c_str(), ipc_magic_.size());
  data32[0] = payload.size();
  data32[1] = type;

  if (::send(fd, header.data(), ipc_header_size_, 0) == -1) {
    throw std::runtime_error("Unable to send IPC header");
  }
  if (::send(fd, payload.c_str(), payload.size(), 0) == -1) {
    throw std::runtime_error("Unable to send IPC payload");
  }
  return Ipc::recv(fd);
}

void Ipc::sendCmd(uint32_t type, const std::string& payload) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto res = Ipc::send(fd_, type, payload);
  signal_cmd.emit(res);
}

void Ipc::subscribe(const std::string& payload) {
  auto res = Ipc::send(fd_event_, IPC_SUBSCRIBE, payload);
  if (res.payload != "{\"success\": true}") {
    throw std::runtime_error("Unable to subscribe ipc event");
  }
}

void Ipc::handleEvent() {
  const auto res = Ipc::recv(fd_event_);
  signal_event.emit(res);
}

}  // namespace waybar::modules::sway
