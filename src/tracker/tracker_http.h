#ifndef LIBTORRENT_TRACKER_TRACKER_HTTP_H
#define LIBTORRENT_TRACKER_TRACKER_HTTP_H

#include "net/address_list.h"
#include <iosfwd>
#include <memory>
#include <tuple>

#include "tracker/tracker_worker.h"
#include "torrent/net/http_get.h"
#include "torrent/system/scheduler.h"
#include "torrent/tracker/tracker_state.h"

namespace torrent {

class Http;

class TrackerHttp : public TrackerWorker {
public:
  static constexpr uint32_t http_timeout = 60;

  TrackerHttp(const TrackerInfo& info, int flags = 0);

  tracker_enum        type() const override;

  void                send_event(tracker::TrackerParams params, tracker::TrackerState::event_enum new_state) override;
  void                send_scrape(tracker::TrackerParams params) override;

  void                close() override;

private:
  void                close_directly();
  void                cleanup() override;

  void                update_requesting_state();

  void                send_event_unsafe(tracker::TrackerState::event_enum state);
  void                send_scrape_unsafe();
  bool                send_next_family(bool scrape = false);

  void                delayed_send_scrape();

  std::stringstream   request_prefix(const std::string& url);
  std::string         request_announce_url(tracker::TrackerState::event_enum state, int family);
  std::tuple<int,int> request_families();

  void                update_tracker_id_unsafe(const std::string& id);

  void                receive_done();
  void                receive_signal_failed(const std::string& msg);
  void                receive_failed(const std::string& msg);

  void                process_failure(const int proto, const Object& object);
  void                process_success(const int proto, const Object& object);
  void                process_scrape(const Object& object);

  tracker::TrackerParams m_params;

  static constexpr int IPV4 = 0;
  static constexpr int IPV6 = 1;
  net::HttpGet                       m_get[2]{};
  std::shared_ptr<std::stringstream> m_data[2]{};
  int wait_close = 0;
  int wait_result = 0;
  std::shared_ptr<torrent::AddressList> proto_result[2];
  void proto_m_close(const int proto);
  void proto_m_slot_failure(const int proto, const std::string& msg);
  void proto_m_slot_success(const int proto, torrent::AddressList&& alist);

  int                 m_hostname_family{};
  int                 m_current_family{};
  int                 m_next_family{};

  bool                m_last_success{};
  std::string         m_last_error_message;

  bool                m_requested_scrape{};

  system::SchedulerEntry m_delay_scrape;
};

} // namespace torrent

#endif
