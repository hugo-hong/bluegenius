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
#define LOG_TAG  "utils_alarm"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <mutex>

#include "utils.h"
#include "allocator.h"
#include "seqlist.h"
#include "event.h"
#include "fixed_queue.h"
#include "reactor.h"
#include "thread.h"
#include "alarm.h"

// Callback and timer threads should run at RT priority in order to ensure they
// meet audio deadlines.  Use this priority for all audio/timer related thread.
static const int THREAD_RT_PRIORITY = 1;


Alarm::Alarm() { 
	CHECK(lazy_initialize());
}

Alarm::~Alarm(){
}

alarm_data_t* Alarm::CreateTimer(const char* name, bool is_periodic) {
	alarm_data_t *timer = static_cast<alarm_data_t*>(sys_malloc(sizeof(alarm_data_t)));

	CHECK(timer != NULL);

	timer->isPeriodic = is_periodic;
	timer->stats.name = sys_strdup(name);
	timer->callback_mutex = new std::recursive_mutex;

	return timer;
}

void Alarm::SetTimer(alarm_data_t *timer, uint64_t interval, callback_func_t cb, void *data) {
	CHECK(timer != NULL);

	std::lock_guard<std::mutex> lock(m_mutex);

	timer->creation_time = Alarm::now();
	timer->period = interval;
	timer->callback_func = cb;
	timer->data = data;

	schedule_next_instance(timer);
	++timer->stats.scheduled_count;
}

bool Alarm::create_timer(const clockid_t clock_id, timer_t* timer) {
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
	sigevt.sigev_notify_function = (void(*)(union sigval))Alarm::timer_callback;
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
        LOG_ERROR(LOG_TAG, "%s unable to allocate alarm list.");
        goto error;
    }
    
    if (!create_timer(CLOCK_BOOTTIME, &m_timer)) goto error;
    timer_initialized = true;
    
    if (!create_timer(CLOCK_BOOTTIME_ALARM, &m_wakeuptimer)) goto error;
    wakeup_timer_initialized = true;
    
    m_alarmExpired = new Event(0);
    
    //callback thread
    m_callbackThread = new Thread("alarm_default_callbacks", SIZE_MAX);
    m_callbackThread->SetRTPriority(THREAD_RT_PRIORITY);
    m_callbackQueue = new FixedQueue(SIZE_MAX);
    m_callbackThread->GetReactor()->Register(m_callbackQueue->GetDequeueFd(), m_callbackQueue, alarm_queue_ready, NULL);
    
    //dispatch thread
    m_dispatchThreadActive = true;
    m_dispatchThread = new Thread("alarm_dispatcher");
    m_dispatchThread->SetRTPriority(THREAD_RT_PRIORITY);
    m_dispatchThread->Post(Alarm::callback_dispatch, this);

    return true;
error:
    if (m_alarmList != NULL) delete m_alarmList;
	if (m_alarmExpired != NULL) delete m_alarmExpired;
    if (m_callbackThread != NULL) delete m_callbackThread;
    if (m_dispatchThread != NULL) delete m_dispatchThread;
	if (timer_initialized) timer_delete(m_timer);
	if (wakeup_timer_initialized) timer_delete(m_wakeuptimer);

    return false;
}

void Alarm::schedule_next_instance(alarm_data_t *alarm) {
}

void Alarm::timer_callback(void *arg) {
}

void Alarm::alarm_queue_ready(void* context) {
}

// Function running on |dispatcher_thread| that performs the following:
//   (1) Receives a signal using |alarm_exired| that the alarm has expired
//   (2) Dispatches the alarm callback for processing by the corresponding
// thread for that alarm.
void Alarm::callback_dispatch(void* context, void* arg) {
    Alarm* alarm = static_cast<Alarm*>(context);
   
}

uint64_t Alarm::now(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_BOOTTIME, &ts) == -1) {
		LOG_ERROR(LOG_TAG, "unable to get current time: %s", strerror(errno));
		return 0;
	}
	return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}


