/*********************************************************************************
   Bluegenius - Bluetooth host protocol stack for Linux/android/windows...
   Copyright (C) 
   Written 2017 by hugo（yongguang hong） <hugo.08@163.com>
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*********************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>

// Callback and timer threads should run at RT priority in order to ensure they
// meet audio deadlines.  Use this priority for all audio/timer related thread.
static const int THREAD_RT_PRIORITY = 1;

typedef struct {
  size_t count;
  uint64_t total_ms;
  uint64_t max_ms;
} stat_t;

// Alarm-related information and statistics
typedef struct {
  const char* name;
  size_t scheduled_count;
  size_t canceled_count;
  size_t rescheduled_count;
  size_t total_updates;
  uint64_t last_update_ms;
  stat_t callback_execution;
  stat_t overdue_scheduling;
  stat_t premature_scheduling;
} alarm_stats_t;

struct alarm_t {
  // The mutex is held while the callback for this alarm is being executed.
  // It allows us to release the coarse-grained monitor lock while a
  // potentially long-running callback is executing. |alarm_cancel| uses this
  // mutex to provide a guarantee to its caller that the callback will not be
  // in progress when it returns.
  std::recursive_mutex* callback_mutex;
  period_ms_t creation_time;
  period_ms_t period;
  period_ms_t deadline;
  period_ms_t prev_deadline;  // Previous deadline - used for accounting of
                              // periodic timers
  bool is_periodic;
  fixed_queue_t* queue;  // The processing queue to add this alarm to
  alarm_callback_t callback;
  void* data;
  alarm_stats_t stats;
};

// If the next wakeup time is less than this threshold, we should acquire
91// a wakelock instead of setting a wake alarm so we're not bouncing in
92// and out of suspend frequently. This value is externally visible to allow
93// unit tests to run faster. It should not be modified by production code.
94int64_t TIMER_INTERVAL_FOR_WAKELOCK_IN_MS = 3000;
95static const clockid_t CLOCK_ID = CLOCK_BOOTTIME;

// This mutex ensures that the |alarm_set|, |alarm_cancel|, and alarm callback
// functions execute serially and not concurrently. As a result, this mutex
// also protects the |alarms| list.
static std::mutex alarms_mutex;
static list_t* alarms;
static timer_t timer;
static bool timer_set;

// All alarm callbacks are dispatched from |dispatcher_thread|
static thread_t* dispatcher_thread;
static bool dispatcher_thread_active;
static event_t* alarm_expired;

// Default alarm callback thread and queue
static thread_t* default_callback_thread;
static fixed_queue_t* default_callback_queue;

alarm_t* alarm_new(const char* name) { 
   return alarm_new_internal(name, false); 
}

static alarm_t* alarm_new_internal(const char* name, bool is_periodic) {
  // Make sure we have a list we can insert alarms into.
  if (!alarms && !lazy_initialize()) {
    CHECK(false);  // if initialization failed, we should not continue
    return NULL;
  }

  alarm_t* ret = static_cast<alarm_t*>(sys_calloc(sizeof(alarm_t)));

  ret->callback_mutex = new std::recursive_mutex;
  ret->is_periodic = is_periodic;
  ret->stats.name = osi_strdup(name);
  // NOTE: The stats were reset by osi_calloc() above

  return ret;
}

static void update_stat(stat_t* stat, uint64_t delta) {
  if (stat->max_ms < delta) stat->max_ms = delta;
  stat->total_ms += delta;
  stat->count++;
}


bool alarm_is_scheduled(const alarm_t* alarm) {
  if ((alarms == NULL) || (alarm == NULL)) return false;
  return (alarm->callback != NULL);
}

static uint64_t now(void) {
  CHECK(alarms != NULL);

  struct timespec ts;
  if (clock_gettime(CLOCK_ID, &ts) == -1) {
    LOG_ERROR(LOG_TAG, "%s unable to get current time: %s", __func__,
              strerror(errno));
    return 0;
  }

  return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

// Remove alarm from internal alarm list and the processing queue
// The caller must hold the |alarms_mutex|
static void remove_pending_alarm(alarm_t* alarm) {
  list_remove(alarms, alarm);
  while (fixed_queue_try_remove_from_queue(alarm->queue, alarm) != NULL) {
    // Remove all repeated alarm instances from the queue.
    // NOTE: We are defensive here - we shouldn't have repeated alarm instances
  }
}
