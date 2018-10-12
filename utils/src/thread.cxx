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
#define LOG_TAG "bluegenius_utils_thread"

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "utils.h"
#include "event.h"
#include "seqlist.h"
#include "reactor.h"
#include "fixed_queue.h"
#include "allocator.h"
#include "thread.h"

struct entry_arg {
	Thread* thread;
	Event* entry_evt;
	int error;
};

typedef struct {
	thread_fn func;
	void* context;
	void* arg;
} work_item_t;

Thread::Thread(const char* name, size_t size)
    :m_isjoined(false)
    ,m_thread(NULL)
    ,m_tid(-1)
    ,m_reactor(NULL)
    ,m_workqueue(NULL)
{       
    New(name, size);
}

Thread::~Thread() {
    Free();
}

void Thread::Post(thread_fn func, void* context, void* arg) {
    CHECK(func != NULL);
    CHECK(m_workqueue != NULL);
    
    // TODO(sharvil): if the current thread == |thread| and we've run out
    // of queue space, we should abort this operation, otherwise we'll
    // deadlock.

    // Queue item is freed either when the queue itself is destroyed
    // or when the item is removed from the queue for dispatch.
    work_item_t* item = (work_item_t*)sys_malloc(sizeof(work_item_t));
    item->func = func;
    item->context = context;
	item->arg = arg;
    m_workqueue->Enqueue(item); 
}

void Thread::Stop() {
    //stop reactor
    if (m_reactor != NULL) m_reactor->Stop();
}

void Thread::Join() {
    if (!std::atomic_exchange(&m_isjoined, true))
        pthread_join(m_thread, NULL);
}

void Thread::New(const char* name, size_t size) {
    CHECK(name != NULL);
    memset(m_name, 0, sizeof(m_name));
    strncpy(m_name, name, THREAD_NAME_MAX);
    
    m_reactor = new Reactor();
    if (NULL == m_reactor) goto error;
    
    m_workqueue = new FixedQueue(size);
    if (NULL == m_workqueue) goto error;
    
    // Start is on the stack, but we use a event, so it's safe
	entry_arg arg;
    arg.thread = this;
    arg.entry_evt = new Event(0);
    arg.error = 0;
    
    pthread_create(&m_thread, NULL, Thread::RunThread, &arg);
    
    arg.entry_evt->Wait();
    delete arg.entry_evt;
    if (arg.error) goto error;
    
    return;
    
error:
	Free();
}

void Thread::Free() {
    Stop();
    Join();
    if (m_reactor) delete m_reactor;
    if (m_workqueue) delete m_workqueue;
}

bool Thread::SetPriority(int priority) {
    if (-1 == m_tid) return false;

    const int rc = setpriority(PRIO_PROCESS, m_tid, priority);
    if (rc < 0) {
        LOG_ERROR(LOG_TAG,
            "unable to set thread priority %d for tid %d, error %d",
            priority, m_tid, rc);
        return false;
    }

    return true;
}

bool Thread::SetRTPriority(int priority) {
    if (-1 == m_tid) return false;

    struct sched_param rt_params;
    rt_params.sched_priority = priority;

    const int rc = sched_setscheduler(m_tid, SCHED_FIFO, &rt_params);
    if (rc != 0) {
        LOG_ERROR(LOG_TAG,
              "unable to set SCHED_FIFO priority %d for tid %d, error %s",
              priority, m_tid, strerror(errno));
        return false;
    }

    return true;
}

bool Thread::IsSelf() {
    CHECK(m_thread != NULL);
    return !!pthread_equal(pthread_self(), m_thread);
}

void* Thread::Run(void* arg) {
    entry_arg* entry = static_cast<entry_arg*>(arg);    

    if (prctl(PR_SET_NAME, (unsigned long)m_name) == -1) {
        LOG_ERROR(LOG_TAG, "unable to set thread name: %s", strerror(errno));
        entry->error = errno;
        entry->entry_evt->Post();
        return NULL;
    }
    
    m_tid = gettid();

	LOG_TRACE(LOG_TAG, "thread id %d, thread name %s started", m_tid, m_name);

    entry->entry_evt->Post();
    //entry->entry_evt has been free, cannot use it anymore

    int fd = m_workqueue->GetDequeueFd(); 
    void* context = m_workqueue;

    reactor_object_t* work_queue_object = m_reactor->Register(fd, context, Thread::WorkqueueReady, NULL);
    m_reactor->Start();
    m_reactor->Unregister(work_queue_object);

    // Make sure we dispatch all queued work items before exiting the thread.
    // This allows a caller to safely tear down by enqueuing a teardown
    // work item and then joining the thread.
    size_t count = 0;
    work_item_t* item = static_cast<work_item_t*>(m_workqueue->TryDequeue());
    while (item && count <= m_workqueue->GetCapcity()) {
        item->func(item->context, item->arg);
        sys_free(item);
        item = static_cast<work_item_t*>(m_workqueue->TryDequeue());
        ++count;
    }

    if (count > m_workqueue->GetCapcity())
		LOG_WARN(LOG_TAG, "growing event queue on shutdown.");

    LOG_TRACE(LOG_TAG, "thread id %d, thread name %s exited",
           m_tid, m_name);
    
    return NULL;
}

void* Thread::RunThread(void* arg) {
    CHECK(arg != NULL);
    entry_arg* entry = static_cast<entry_arg*>(arg);    
    return entry->thread->Run(arg);
}


void Thread::WorkqueueReady(void* context) {
  CHECK(context != NULL);

  FixedQueue* queue = (FixedQueue*)context;
  work_item_t* item = static_cast<work_item_t*>(queue->Dequeue());
  item->func(item->context, item->arg);
  sys_free(item);
}
