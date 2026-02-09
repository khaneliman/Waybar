#include "util/netdev_parser.hpp"

#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <sstream>

TEST_CASE("netdev parser skips malformed rows", "[util][network][netdev]") {
  std::istringstream netdev(
      "Inter-|   Receive                                                |  Transmit\n"
      " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets "
      "errs drop fifo colls carrier compressed\n"
      "lo: not-a-number\n"
      "lo:\n"
      "lo 100 1 0 0 0 0 0 0 200 2 0 0 0 0 0 0\n"
      "lo: 100 1 0 0 0 0 0 0 200 2 0 0 0 0 0 0\n"
      "lo: 300 1 0 0 0 0 0 0 missing-tx\n");

  const auto [rx, tx] = waybar::util::read_netdev_bytes(netdev, "lo");

  REQUIRE(rx == 100);
  REQUIRE(tx == 200);
}

TEST_CASE("netdev parser aggregates matching interfaces only", "[util][network][netdev]") {
  std::istringstream netdev(
      "Inter-|   Receive                                                |  Transmit\n"
      " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets "
      "errs drop fifo colls carrier compressed\n"
      "eth0: 30 1 0 0 0 0 0 0 40 1 0 0 0 0 0 0\n"
      "wlan0: 50 2 0 0 0 0 0 0 60 2 0 0 0 0 0 0\n"
      "eth0: 70 3 0 0 0 0 0 0 80 3 0 0 0 0 0 0\n");

  const auto [rx, tx] = waybar::util::read_netdev_bytes(netdev, "eth0");

  REQUIRE(rx == 100);
  REQUIRE(tx == 120);
}
