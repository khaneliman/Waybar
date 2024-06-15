#include "MockSystemCalls.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

MockSystemCalls::MockSystemCalls() {
  instance = this;
  socketPtr = MockSystemCalls::socket;
  getaddrinfoPtr = MockSystemCalls::getaddrinfo;
  connectPtr = MockSystemCalls::connect;
  writePtr = MockSystemCalls::write;
  readPtr = MockSystemCalls::read;
  closePtr = MockSystemCalls::close;
  getenvPtr = MockSystemCalls::getenv;
  snprintfPtr = MockSystemCalls::snprintf;
}

int MockSystemCalls::socket(int domain, int type, int protocol) {
  return instance->mock.get().socket(domain, type, protocol);
}

int MockSystemCalls::getaddrinfo(const char* node, const char* service,
                                 const struct addrinfo* hints, struct addrinfo** res) {
  return instance->mock.get().getaddrinfo(node, service, hints, res);
}

int MockSystemCalls::connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return instance->mock.get().connect(sockfd, addr, addrlen);
}

ssize_t MockSystemCalls::write(int fd, const void* buf, size_t count) {
  return instance->mock.get().write(fd, buf, count);
}

ssize_t MockSystemCalls::read(int fd, void* buf, size_t count) {
  return instance->mock.get().read(fd, buf, count);
}

int MockSystemCalls::close(int fd) { return instance->mock.get().close(fd); }

char* MockSystemCalls::getenv(const char* name) { return instance->mock.get().getenv(name); }

int MockSystemCalls::snprintf(char* str, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf(str, size, format, args);
  va_end(args);
  return result;
}

// Mocked functions need to be in an extern "C" block for C linkage.
extern "C" {
int socket(int domain, int type, int protocol) {
  return MockSystemCalls::socketPtr(domain, type, protocol);
}

int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints,
                struct addrinfo** res) {
  return MockSystemCalls::getaddrinfoPtr(node, service, hints, res);
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return MockSystemCalls::connectPtr(sockfd, addr, addrlen);
}

ssize_t write(int fd, const void* buf, size_t count) {
  return MockSystemCalls::writePtr(fd, buf, count);
}

ssize_t read(int fd, void* buf, size_t count) { return MockSystemCalls::readPtr(fd, buf, count); }

int close(int fd) { return MockSystemCalls::closePtr(fd); }

char* getenv(const char* name) { return MockSystemCalls::getenvPtr(name); }

int snprintf(char* str, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = MockSystemCalls::snprintfPtr(str, size, format, args);
  va_end(args);
  return result;
}
}
