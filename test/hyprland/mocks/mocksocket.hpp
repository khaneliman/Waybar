#ifndef MOCKSYSTEMCALLS_HPP
#define MOCKSYSTEMCALLS_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <fakeit.hpp>

using namespace fakeit;

class MockSystemCalls {
 public:
  Mock<GlobalNamespace> mock;

  MockSystemCalls();

  static int socket(int domain, int type, int protocol);
  static int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints,
                         struct addrinfo** res);
  static int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  static ssize_t write(int fd, const void* buf, size_t count);
  static ssize_t read(int fd, void* buf, size_t count);
  static int close(int fd);
  static char* getenv(const char* name);
  static int snprintf(char* str, size_t size, const char* format, ...);
};

#endif  // MOCKSYSTEMCALLS_HPP
