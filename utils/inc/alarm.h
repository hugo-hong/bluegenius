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
#ifndef _UTIL_ALARM_H_
#define _UTIL_ALARM_H_

// Prototype for the alarm callback function.
typedef void(*callback_func_t)(void* data);
typedef struct {
	size_t count;
	uint64_t total_ms;
	uint64_t max_ms;
}stat_t;

typedef struct {
	const char *name;
	size_t scheduled_count;
	size_t canceled_count;
	size_t rescheduled_count;
	size_t total_updates;
	uint64_t last_update_ms;
	stat_t overdue_scheduling;
	stat_t premature_scheduling;
}alarm_stats_t;

typedef struct {
	bool isPeriodic;
	uint64_t creation_time;
	uint64_t period;
	uint64_t deadline;
	uint64_t prev_deadline;
	FixedQueue *queue;
	std::recursive_mutex *callback_mutex;
	callback_func_t callback_func;
	void *data;
	alarm_stats_t stats;
}alarm_data_t;

class Alarm {
public:
	Alarm();
	~Alarm();	

	alarm_data_t* CreateTimer(const char* name, bool is_periodic);
	void SetTimer(alarm_data_t *timer, uint64_t interval, callback_func_t cb, void *data);
protected:	

	bool lazy_initialize(void);
	bool create_timer(const clockid_t clock_id, timer_t* timer);
	void schedule_next_instance(alarm_data_t *alarm);

	static uint64_t now(void);
	static void timer_callback(void *arg);
	static void alarm_queue_ready(void* context);
	static void callback_dispatch(void* context, void* arg);

private:	
	alarm_data_t m_alarm;
	
	std::mutex m_mutex;
	SeqList *m_alarmList;
	timer_t m_timer;
	timer_t m_wakeuptimer;
	Event *m_alarmExpired;
	Thread *m_callbackThread;
	FixedQueue *m_callbackQueue;
	bool m_dispatchThreadActive;
	Thread *m_dispatchThread;
	
};

#endif //_UTIL_ALARM_H_

