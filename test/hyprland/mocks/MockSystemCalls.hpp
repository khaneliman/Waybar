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
  MockSystemCalls();

  Mock<MockSystemCalls> mock;

  static int socket(int domain, int type, int protocol);
  static int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints,
                         struct addrinfo** res);
  static int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  static ssize_t write(int fd, const void* buf, size_t count);
  static ssize_t read(int fd, void* buf, size_t count);
  static int close(int fd);
  static char* getenv(const char* name);
  static int snprintf(char* str, size_t size, const char* format, ...);

  static inline int (*socketPtr)(int, int, int) = nullptr;
  static inline int (*getaddrinfoPtr)(const char*, const char*, const struct addrinfo*,
                                      struct addrinfo**) = nullptr;
  static inline int (*connectPtr)(int, const struct sockaddr*, socklen_t) = nullptr;
  static inline ssize_t (*writePtr)(int, const void*, size_t) = nullptr;
  static inline ssize_t (*readPtr)(int, void*, size_t) = nullptr;
  static inline int (*closePtr)(int) = nullptr;
  static inline char* (*getenvPtr)(const char*) = nullptr;
  static inline int (*snprintfPtr)(char*, size_t, const char*, ...) = nullptr;
};

#endif  // MOCKSYSTEMCALLS_HPP
