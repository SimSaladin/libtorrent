#include "config.h"

#include "thread_tracker.h"

#include <cassert>

#include "torrent/exceptions.h"
#include "torrent/tracker/manager.h"
#include "utils/instrumentation.h"

namespace torrent {

std::atomic<ThreadTracker*> ThreadTracker::m_thread_tracker{nullptr};

ThreadTracker::~ThreadTracker() = default;

void
ThreadTracker::create_thread(utils::Thread* main_thread) {
  assert(m_thread_tracker == nullptr);

  m_thread_tracker = new ThreadTracker();
  m_thread_tracker.load()->m_tracker_manager = std::make_unique<tracker::Manager>(main_thread, m_thread_tracker);
}

ThreadTracker*
ThreadTracker::thread_tracker() {
  return m_thread_tracker;
}

void
ThreadTracker::init_thread() {
  m_state = STATE_INITIALIZED;

  m_instrumentation_index = INSTRUMENTATION_POLLING_DO_POLL_TRACKER - INSTRUMENTATION_POLLING_DO_POLL;
}

void
ThreadTracker::cleanup_thread() {
  m_thread_tracker = nullptr;

  m_tracker_manager.reset();
}

void
ThreadTracker::call_events() {
  // lt_log_print_locked(torrent::LOG_THREAD_NOTICE, "Got ThreadTracker tick.");

  // TODO: Consider moving this into timer events instead.
  if ((m_flags & flag_do_shutdown)) {
    if ((m_flags & flag_did_shutdown))
      throw internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw shutdown_exception();
  }

  // TODO: Do we need to process scheduled events here?

  process_callbacks();
}

std::chrono::microseconds
ThreadTracker::next_timeout() {
  return std::chrono::microseconds(10s);
}

} // namespace torrent
