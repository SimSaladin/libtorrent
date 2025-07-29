#include "config.h"

#include "tracker/tracker_worker.h"

#include "manager.h"
#include "torrent/exceptions.h"
#include "torrent/connection_manager.h"

namespace torrent {

TrackerWorker::~TrackerWorker() = default;

// TODO: Refactor this to return a struct containing bind/local ip, etc.
// TODO: Have separate prefer-local ip protocol, and detect local ipv6 address handling.

// TODO: TrackerController should immediately check normal_ip_preference to check if it should try
// the next tracker. This should be a special handling of tracker send event failure.

TrackerWorker::ip_type
TrackerWorker::normal_ip_preference() {
  bool is_block_ipv4 = manager->connection_manager()->is_block_ipv4();
  bool is_block_ipv6 = manager->connection_manager()->is_block_ipv6();
  bool is_prefer_ipv4 = false;
  bool is_prefer_ipv6 = manager->connection_manager()->is_prefer_ipv6();

  if (is_block_ipv4 && is_block_ipv6)
    return IP_NONE;

  if (is_block_ipv4) {
    if (is_prefer_ipv6)
      return IP_NONE;

    return IP_USE_V6;
  }

  if (is_block_ipv6) {
    if (is_prefer_ipv4)
      return IP_NONE;

    return IP_USE_V4;
  }

  if (is_prefer_ipv4)
    return IP_USE_V4;

  if (is_prefer_ipv6)
    return IP_USE_V6;

  return IP_EITHER;
}

TrackerWorker::ip_type
TrackerWorker::fallback_ip_preference() {
  bool is_block_ipv4 = manager->connection_manager()->is_block_ipv4();
  bool is_block_ipv6 = manager->connection_manager()->is_block_ipv6();
  bool is_prefer_ipv4 = false;
  bool is_prefer_ipv6 = manager->connection_manager()->is_prefer_ipv6();

  if (is_block_ipv4 && is_block_ipv6)
    return IP_NONE;

  if (is_block_ipv6)
    return IP_USE_V4;

  if (is_block_ipv4)
    return IP_USE_V6;

  if (is_prefer_ipv4)
    return IP_PREFER_V4;

  if (is_prefer_ipv6)
    return IP_PREFER_V6;

  return IP_EITHER;
}

}  // namespace torrent
