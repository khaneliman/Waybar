#include "util/netdev_parser.hpp"

#include <sstream>

std::pair<unsigned long long, unsigned long long> waybar::util::read_netdev_bytes(
    std::istream& netdev, const std::string& ifname) {
  std::string line;
  // Skip /proc/net/dev header rows.
  std::getline(netdev, line);
  std::getline(netdev, line);

  unsigned long long received_bytes = 0ull;
  unsigned long long transmitted_bytes = 0ull;

  while (std::getline(netdev, line)) {
    std::istringstream iss(line);

    std::string iface_name;
    if (!(iss >> iface_name) || iface_name.empty() || iface_name.back() != ':') {
      continue;
    }
    iface_name.pop_back();
    if (iface_name != ifname) {
      continue;
    }

    unsigned long long received = 0ull;
    unsigned long long transmitted = 0ull;
    if (!(iss >> received)) {
      continue;
    }

    unsigned long long ignored_col = 0ull;
    bool parsed_ok = true;
    for (int cols_to_skip = 0; cols_to_skip < 7; ++cols_to_skip) {
      if (!(iss >> ignored_col)) {
        parsed_ok = false;
        break;
      }
    }
    if (!parsed_ok || !(iss >> transmitted)) {
      continue;
    }

    received_bytes += received;
    transmitted_bytes += transmitted;
  }

  return {received_bytes, transmitted_bytes};
}
