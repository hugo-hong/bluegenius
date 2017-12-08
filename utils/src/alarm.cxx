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



// If the next wakeup time is less than this threshold, we should acquire
// a wakelock instead of setting a wake alarm so we're not bouncing in
// and out of suspend frequently. This value is externally visible to allow
// unit tests to run faster. It should not be modified by production code.
int64_t TIMER_INTERVAL_FOR_WAKELOCK_IN_MS = 3000;
static const clockid_t CLOCK_ID = CLOCK_BOOTTIME;

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


Alarm::Alarm(const char* name, bool is_periodic)
    :m_creation_time(0)
    ,m_period(0)
    ,m_deadline(0)
    ,m_prev_deadline(0)
    ,m_isPeriodic(false)
    ,m_queue(NULL)
    ,m_callback(NULL)
    ,m_data(NULL)
    ,m_mutex(NULL)
    ,m_alarmList(NULL)
{
    New(name, is_periodic);
}

bool Alarm::CreateTimer(const clockid_t clock_id, timer_t* timer) {
    CHECK(timer == NULL);
    
    //create timer with rt priority thread
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
    struct sched_param param;
    param.sched_priority = THREAD_RT_PRIORITY;
    pthread_attr_setschedparam(&thread_attr, &param);
    
    struct sigevent sigevt;
    memset(&sigevt, 0, sizeof(sigevt));
    sigevt.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))timer_callback;  
    if (timer_create(clock_id, &sigevt, timer) == -1) {
        LOG_ERROR(LOG_TAG, "%s unable to create timer with clock %d: %s", __func__,
                  clock_id, strerror(errno));
        if (clock_id == CLOCK_BOOTTIME_ALARM) {
          LOG_ERROR(LOG_TAG,
                    "The kernel might not have support for "
                    "timer_create(CLOCK_BOOTTIME_ALARM): "
                    "https://lwn.net/Articles/429925/");
          LOG_ERROR(LOG_TAG,
                    "See following patches: "
                    "https://git.kernel.org/cgit/linux/kernel/git/torvalds/"
                    "linux.git/log/?qt=grep&q=CLOCK_BOOTTIME_ALARM");
        }
        return false;
  }

  return true;

}


bool Alarm::lazy_initialize(void) {
    CHECK(m_alarmList == NULL);

    // timer_t doesn't have an invalid value so we must track whether
    // the |timer| variable is valid ourselves.
    bool timer_initialized = false;
    bool wakeup_timer_initialized = false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_alarmList = new SeqList(NULL);
    if (NULL == m_alarmList) {
        LOG_ERROR(LOG_TAG, "%s unable to allocate alarm list.", __func__);
        goto error;
    }
    
    


    
    if (!timer_create_internal(CLOCK_ID, &timer)) goto error;
    timer_initialized = true;

    if (!timer_create_internal(CLOCK_ID_ALARM, &wakeup_timer)) goto error;
    wakeup_timer_initialized = true;

    alarm_expired = event_new(0);
    if (!alarm_expired) {
    LOG_ERROR(LOG_TAG, "%s unable to create alarm expired semaphore", __func__);
    goto error;
    }

    default_callback_thread =
      thread_new_sized("alarm_default_callbacks", SIZE_MAX);
    if (default_callback_thread == NULL) {
    LOG_ERROR(LOG_TAG, "%s unable to create default alarm callbacks thread.",
              __func__);
    goto error;
    }
    thread_set_rt_priority(default_callback_thread, THREAD_RT_PRIORITY);
    default_callback_queue = fixed_queue_new(SIZE_MAX);
    if (default_callback_queue == NULL) {
    LOG_ERROR(LOG_TAG, "%s unable to create default alarm callbacks queue.",
              __func__);
    goto error;
    }
    alarm_register_processing_queue(default_callback_queue,
                                  default_callback_thread);

    dispatcher_thread_active = true;
    dispatcher_thread = thread_new("alarm_dispatcher");
    if (!dispatcher_thread) {
    LOG_ERROR(LOG_TAG, "%s unable to create alarm callback thread.", __func__);
    goto error;
    }
    thread_set_rt_priority(dispatcher_thread, THREAD_RT_PRIORITY);
    thread_post(dispatcher_thread, callback_dispatch, NULL);
    return true;

    error:
    fixed_queue_free(default_callback_queue, NULL);
    default_callback_queue = NULL;
    thread_free(default_callback_thread);
    default_callback_thread = NULL;

    thread_free(dispatcher_thread);
    dispatcher_thread = NULL;

    dispatcher_thread_active = false;

    semaphore_free(alarm_expired);
    alarm_expired = NULL;

    if (wakeup_timer_initialized) timer_delete(wakeup_timer);

    if (timer_initialized) timer_delete(timer);

    list_free(alarms);
    alarms = NULL;

    return false;
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
  ret->stats.name = sys_strdup(name);
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


static bool lazy_initialize(void) {
  CHECK(alarms == NULL);

  // timer_t doesn't have an invalid value so we must track whether
  // the |timer| variable is valid ourselves.
  bool timer_initialized = false;
  bool wakeup_timer_initialized = false;

  std::lock_guard<std::mutex> lock(alarms_mutex);

  alarms = list_new(NULL);
  if (!alarms) {
    LOG_ERROR(LOG_TAG, "%s unable to allocate alarm list.", __func__);
    goto error;
  }

  if (!timer_create_internal(CLOCK_ID, &timer)) goto error;
  timer_initialized = true;

  if (!timer_create_internal(CLOCK_ID_ALARM, &wakeup_timer)) goto error;
  wakeup_timer_initialized = true;

  alarm_expired = event_new(0);
  if (!alarm_expired) {
    LOG_ERROR(LOG_TAG, "%s unable to create alarm expired semaphore", __func__);
    goto error;
  }

  default_callback_thread =
      thread_new_sized("alarm_default_callbacks", SIZE_MAX);
  if (default_callback_thread == NULL) {
    LOG_ERROR(LOG_TAG, "%s unable to create default alarm callbacks thread.",
              __func__);
    goto error;
  }
  thread_set_rt_priority(default_callback_thread, THREAD_RT_PRIORITY);
  default_callback_queue = fixed_queue_new(SIZE_MAX);
  if (default_callback_queue == NULL) {
    LOG_ERROR(LOG_TAG, "%s unable to create default alarm callbacks queue.",
              __func__);
    goto error;
  }
  alarm_register_processing_queue(default_callback_queue,
                                  default_callback_thread);

  dispatcher_thread_active = true;
  dispatcher_thread = thread_new("alarm_dispatcher");
  if (!dispatcher_thread) {
    LOG_ERROR(LOG_TAG, "%s unable to create alarm callback thread.", __func__);
    goto error;
  }
  thread_set_rt_priority(dispatcher_thread, THREAD_RT_PRIORITY);
  thread_post(dispatcher_thread, callback_dispatch, NULL);
  return true;

error:
  fixed_queue_free(default_callback_queue, NULL);
  default_callback_queue = NULL;
  thread_free(default_callback_thread);
  default_callback_thread = NULL;

  thread_free(dispatcher_thread);
  dispatcher_thread = NULL;

  dispatcher_thread_active = false;

  semaphore_free(alarm_expired);
  alarm_expired = NULL;

  if (wakeup_timer_initialized) timer_delete(wakeup_timer);

  if (timer_initialized) timer_delete(timer);

  list_free(alarms);
  alarms = NULL;

  return false;
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
