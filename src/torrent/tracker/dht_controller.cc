#include "config.h"

#include "dht_controller.h"

#include <cassert>

#include "dht/dht_router.h"
#include "src/manager.h"
#include "torrent/connection_manager.h"
#include "torrent/exceptions.h"
#include "torrent/throttle.h"
#include "torrent/net/socket_address.h"
#include "torrent/utils/log.h"
#include "torrent/utils/thread.h"

#define LT_LOG_THIS(log_fmt, ...)                                       \
  lt_log_print_subsystem(torrent::LOG_DHT_MANAGER, "dht_manager", log_fmt, __VA_ARGS__);

namespace torrent::tracker {

DhtController::DhtController(utils::Thread* owner_thread) :
    m_owner_thread(owner_thread) {
}

DhtController::~DhtController() {
  assert(std::this_thread::get_id() == m_owner_thread->thread_id());
  assert(m_router && !m_router->is_active() == false);
}

void
DhtController::initialize(const Object& dht_cache, const sockaddr* bind_address) {
  LT_LOG_THIS("initializing : bind_address:%s", sa_pretty_str(bind_address).c_str());

  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (m_router != nullptr)
    throw internal_error("DhtController::initialize called with DHT already active.");

  try {
    m_router = std::make_unique<DhtRouter>(dht_cache, rak::socket_address::cast_from(bind_address));

  } catch (const torrent::local_error& e) {
    LT_LOG_THIS("initialization failed (error:%s)", e.what());

    // TODO: Handle error properly.
  }
}

bool
DhtController::start(ConnectionManager::port_type port) {
  LT_LOG_THIS("starting (port:%d)", port);

  if (is_active())
    throw input_error("DHT already active, cannot start again.");

  // m_owner_thread->callback_interrupt_pollling

  // Use atomic bool to return result.

  auto lock = std::scoped_lock(m_lock);

  if (m_router == nullptr)
    throw internal_error("DhtController::start called without initializing first.");

  if (m_active)
    throw internal_error("DhtController::start called while DHT already active.");

  try {
    m_port = port;
    m_router->start(port);
    m_active = true;

    return true;

  } catch (const torrent::local_error& e) {
    LT_LOG_THIS("start failed (error:%s)", e.what());
    return false;
  }
}

void
DhtController::stop() {
  if (!is_active())
    return;

  LT_LOG_THIS("stopping", 0);

  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (m_router == nullptr || !m_active)
    return;

  m_router->stop();
  m_active = false;
}

bool
DhtController::is_valid() {
  auto lock = std::scoped_lock(m_lock);

  return m_router != nullptr;
}

bool
DhtController::is_active() {
  auto lock = std::scoped_lock(m_lock);

  return m_router && m_active;
}

bool
DhtController::is_receiving_requests() {
  auto lock = std::scoped_lock(m_lock);

  return m_receive_requests;
}

uint16_t
DhtController::port() {
  auto lock = std::scoped_lock(m_lock);

  return m_port;
}

void
DhtController::set_receive_requests(bool state) {
  auto lock = std::scoped_lock(m_lock);

  m_receive_requests = state;
}

void
DhtController::add_node(const sockaddr* sa, int port) {
  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (m_router)
    m_router->contact(sa, port);
}

void
DhtController::add_node(const std::string& host, int port) {
  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (m_router)
    m_router->add_contact(host, port);
}

Object*
DhtController::store_cache(Object* container) {
  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (!m_router)
    throw internal_error("DhtController::store_cache called but DHT not initialized.");

  return m_router->store_cache(container);
}

DhtController::statistics_type
DhtController::get_statistics() const {
  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (!m_router)
    throw internal_error("DhtController::get_statistics called but DHT not initialized.");

  return m_router->get_statistics();
}

void
DhtController::reset_statistics() {
  // m_owner_thread->callback_interrupt_pollling

  auto lock = std::scoped_lock(m_lock);

  if (!m_router)
    throw internal_error("DhtController::reset_statistics called but DHT not initialized.");

  m_router->reset_statistics();
}

// TODO: Throttle needs to be made thread-safe.

// TODO: Don't allow setting throttle for DHT due to threading.

void
DhtController::set_upload_throttle([[maybe_unused]] Throttle* t) {
  // if (!m_router)
  //   throw internal_error("DhtController::set_upload_throttle() called but DHT not initialized.");

  // if (m_router->is_active())
  //   throw internal_error("DhtController::set_upload_throttle() called while DHT server active.");

  // m_router->set_upload_throttle(t->throttle_list());
}

void
DhtController::set_download_throttle([[maybe_unused]] Throttle* t) {
  // if (!m_router)
  //   throw internal_error("DhtController::set_download_throttle() called but DHT not initialized.");

  // if (m_router->is_active())
  //   throw internal_error("DhtController::set_download_throttle() called while DHT server active.");

  // m_router->set_download_throttle(t->throttle_list());
}

void
DhtController::shutdown() {
  assert(std::this_thread::get_id() == m_owner_thread->thread_id());

  stop();
}

void
DhtController::announce(const HashString& info_hash, TrackerDht* tracker) {
  assert(std::this_thread::get_id() == m_owner_thread->thread_id());

  if (!m_router)
    throw internal_error("DhtController::announce called but DHT not initialized.");

  m_router->announce(info_hash, tracker);
}

void
DhtController::cancel_announce(const HashString* info_hash, const torrent::TrackerDht* tracker) {
  assert(std::this_thread::get_id() == m_owner_thread->thread_id());

  if (!m_router)
    throw internal_error("DhtController::cancel_announce called but DHT not initialized.");

  m_router->cancel_announce(info_hash, tracker);
}

} // namespace torrent::tracker
