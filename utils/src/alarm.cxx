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
    
    if (!CreateTimer(CLOCK_ID, &m_timer)) goto error;
    timer_initialized = true;
    
    if (!CreateTimer(CLOCK_ID_ALARM, &m_wakeuptimer)) goto error;
    wakeup_timer_initialized = true;
    
    m_alarmExpired = new Event(0);
    

    return true;
error:
    return false;
}





