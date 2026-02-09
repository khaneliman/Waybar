#pragma once

#include <istream>
#include <string>
#include <utility>

namespace waybar::util {

std::pair<unsigned long long, unsigned long long> read_netdev_bytes(std::istream& netdev,
                                                                    const std::string& ifname);

}  // namespace waybar::util
