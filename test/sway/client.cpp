#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#include "modules/sway/ipc/client.hpp"

namespace sway = waybar::modules::sway;

class IpcTestAccessor : public sway::Ipc {
 public:
  using sway::Ipc::parseSocketPathOutput;
  using sway::Ipc::readSocketPathFromStream;
};

TEST_CASE("sway ipc socket path parser trims trailing newlines", "[sway][ipc]") {
  REQUIRE(IpcTestAccessor::parseSocketPathOutput("/tmp/sway.sock\n") == "/tmp/sway.sock");
  REQUIRE(IpcTestAccessor::parseSocketPathOutput("/tmp/sway.sock\r\n") == "/tmp/sway.sock");
}

TEST_CASE("sway ipc socket path parser rejects empty output", "[sway][ipc]") {
  REQUIRE_THROWS_AS(IpcTestAccessor::parseSocketPathOutput(""), std::runtime_error);
  REQUIRE_THROWS_AS(IpcTestAccessor::parseSocketPathOutput("\n"), std::runtime_error);
}

TEST_CASE("sway ipc stream reader appends only bytes read", "[sway][ipc]") {
  std::unique_ptr<FILE, int (*)(FILE*)> tmp(tmpfile(), fclose);
  REQUIRE(tmp != nullptr);

  const std::string expected = "/tmp/sway-ipc.1000.1234.sock";
  REQUIRE(fputs((expected + "\n").c_str(), tmp.get()) >= 0);
  REQUIRE(fflush(tmp.get()) == 0);
  rewind(tmp.get());

  const auto parsed = IpcTestAccessor::readSocketPathFromStream(tmp.get());
  REQUIRE(parsed == expected);
}
