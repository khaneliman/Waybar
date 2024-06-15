#include <cstdlib>
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include <fakeit.hpp>

#include "fixtures/IPCTestFixture.hpp"
#include "mocks/mocksystemcalls.hpp"
#include "modules/hyprland/backend.hpp"

namespace fs = std::filesystem;
namespace hyprland = waybar::modules::hyprland;

TEST_CASE_METHOD(IPCTestFixture, "XDGRuntimeDirExists", "[getSocketFolder]") {
  // Test case: XDG_RUNTIME_DIR exists and contains "hypr" directory
  // Arrange
  tempDir = fs::temp_directory_path() / "hypr_test/run/user/1000";
  fs::path expectedPath = tempDir / "hypr" / instanceSig;
  fs::create_directories(tempDir / "hypr" / instanceSig);
  setenv("XDG_RUNTIME_DIR", tempDir.c_str(), 1);

  // Act
  fs::path actualPath = getSocketFolder(instanceSig);

  // Assert expected result
  REQUIRE(actualPath == expectedPath);
}

TEST_CASE_METHOD(IPCTestFixture, "XDGRuntimeDirDoesNotExist", "[getSocketFolder]") {
  // Test case: XDG_RUNTIME_DIR does not exist
  // Arrange
  unsetenv("XDG_RUNTIME_DIR");
  fs::path expectedPath = fs::path("/tmp") / "hypr" / instanceSig;

  // Act
  fs::path actualPath = getSocketFolder(instanceSig);

  // Assert expected result
  REQUIRE(actualPath == expectedPath);
}

TEST_CASE_METHOD(IPCTestFixture, "XDGRuntimeDirExistsNoHyprDir", "[getSocketFolder]") {
  // Test case: XDG_RUNTIME_DIR exists but does not contain "hypr" directory
  // Arrange
  fs::path tempDir = fs::temp_directory_path() / "hypr_test/run/user/1000";
  fs::create_directories(tempDir);
  setenv("XDG_RUNTIME_DIR", tempDir.c_str(), 1);
  fs::path expectedPath = fs::path("/tmp") / "hypr" / instanceSig;

  // Act
  fs::path actualPath = getSocketFolder(instanceSig);

  // Assert expected result
  REQUIRE(actualPath == expectedPath);
}

TEST_CASE("IPC::getSocket1Reply - Successful execution") {
  MockSystemCalls mockSystemCalls;

  std::string rq = "request";
  std::string expectedResponse = "response";

  When(Method(mockSystemCalls.mock, socket).Using(AF_UNIX, SOCK_STREAM, 0)).Return(3);

  When(Method(mockSystemCalls.mock, getaddrinfo).Using("localhost", nullptr, _, _)).Return(0);

  When(Method(mockSystemCalls.mock, getenv).Using("HYPRLAND_INSTANCE_SIGNATURE"))
      .Return((char*)"signature");

  When(Method(mockSystemCalls.mock, snprintf).Using(_, _, _, _)).Return(0);

  When(Method(mockSystemCalls.mock, connect).Using(3, _, sizeof(sockaddr_un))).Return(0);

  When(Method(mockSystemCalls.mock, write).Using(3, rq.c_str(), rq.length())).Return(rq.length());

  When(Method(mockSystemCalls.mock, read).Using(3, _, 8192))
      .Return(expectedResponse.length())
      .Then(Do([&expectedResponse](int, void* buf, size_t) -> ssize_t {
        std::memcpy(buf, expectedResponse.c_str(), expectedResponse.length());
        return expectedResponse.length();
      }))
      .AlwaysReturn(0);

  When(Method(mockSystemCalls.mock, close).Using(3)).Return(0);

  auto result = IPCTestFixture::getSocket1Reply(rq);
  REQUIRE(result == expectedResponse);
}

TEST_CASE("IPC::getSocket1Reply - Socket creation failure") {
  MockSystemCalls mockSystemCalls;

  When(Method(mockSystemCalls.mock, socket).Using(AF_UNIX, SOCK_STREAM, 0)).Return(-1);

  auto result = IPCTestFixture::getSocket1Reply("request");
  REQUIRE(result.empty());
}
