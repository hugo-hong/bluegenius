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

#define LOG_TAG "thread_proxy"

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>

#include"thread.h"

typedef struct {
  Thread* thread;
  Event* entry_evt;
  int error;
}entry_arg;

typedef struct {
  thread_fn func;
  void* context;
} work_item_t;

Thread::Thread(const char* name, size_t size)
    :m_isJoined(false)
    ,m_pthread(NULL)
    ,m_tid(-1)
    ,m_pReactor(NULL)
    ,m_pWorkQueue(NULL)
{       
    New(name, size);
}

Thread::~Thread() {
    Free();
}

void Thread::Post(thread_fn func, void* context) {
    CHECK(func != NULL);
    CHECK(m_pWorkQueue != NULL);
    
    // TODO(sharvil): if the current thread == |thread| and we've run out
    // of queue space, we should abort this operation, otherwise we'll
    // deadlock.

    // Queue item is freed either when the queue itself is destroyed
    // or when the item is removed from the queue for dispatch.
    work_item_t* item = (work_item_t*)sys_malloc(sizeof(work_item_t));
    item->func = func;
    item->context = context;
    m_pWorkQueue->Enqueue(item); 
}

void Thread::Stop() {
    //stop reactor
    if (m_pReactor != NULL) m_pReactor->Stop();
}

void Thread::Join() {
    if (!std::atomic_exchange(&m_isJoined, true))
        pthread_join(m_pthread, NULL);
}

void Thread::New(const char* name, size_t size) {
    CHECK(name != NULL);
    memset(m_name, 0, sizeof(m_name));
    strncpy(m_name, name, THREAD_NAME_MAX);
    
    m_pReactor = new Reactor();
    if (NULL == m_pReactor) goto error;
    
    m_pWorkQueue = new FixedQueue(size);
    if (NULL == m_pWorkQueue) goto error;
    
    // Start is on the stack, but we use a event, so it's safe
    entry_arg arg;
    arg.thread = this;
    arg.entry_evt = new Event(0);
    arg.error = 0;
    
    pthread_create(&m_pthread, NULL, Thread::run_thread, &arg);
    
    arg.entry_evt->Wait();
    delete arg.entry_evt;
    if (arg.error) goto error;
    
    return;
    
error:
    free();
}

void Thread::Free() {
    Stop();
    Join();
    if (m_pReactor) delete m_pReactor;
    if (m_pWorkQueue) delete m_pWorkQueue;
}

bool Thread::SetPriority(int priority) {
    if (-1 == m_tid) return false;

    const int rc = setpriority(PRIO_PROCESS, m_tid, priority);
    if (rc < 0) {
        LOG_ERROR(LOG_TAG,
            "%s unable to set thread priority %d for tid %d, error %d",
            __func__, priority, m_tid, rc);
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
              "%s unable to set SCHED_FIFO priority %d for tid %d, error %s",
              __func__, priority, m_tid, strerror(errno));
        return false;
    }

    return true;
}

bool Thread::IsSelf() {
    CHECK(m_pthread != NULL);
    return !!pthread_equal(pthread_self(), m_pthread);
}

void* Thread::Run(void* arg) {
    struct entry_arg* entry = static_cast<entry_arg*>(arg);    

    if (prctl(PR_SET_NAME, (unsigned long)m_name) == -1) {
        LOG_ERROR(LOG_TAG, "%s unable to set thread name: %s", __func__,
              strerror(errno));
        entry->error = errno;
        entry->entry_evt->Post();
        return NULL;
    }
    
    m_tid = gettid();

    LOG_INFO(LOG_TAG, "%s: thread id %d, thread name %s started", __func__,
           m_tid, m_name);

    entry->entry_evt->Post();
    //entry->entry_evt has been free, cannot use it anymore

    int fd = m_pWorkQueue->GetDequeue(); 
    void* context = m_pWorkQueue;

    reactor_object_t* work_queue_object = m_pReactor->Register(fd, context, work_queue_read_cb, NULL);
    m_pReactor->Start();
    m_pReactor->Deregister(work_queue_object);

    // Make sure we dispatch all queued work items before exiting the thread.
    // This allows a caller to safely tear down by enqueuing a teardown
    // work item and then joining the thread.
    size_t count = 0;
    work_item_t* item = static_cast<work_item_t*>(m_pWorkQueue->TryDequeue());
    while (item && count <= m_pWorkQueue->Capacity()) {
        item->func(item->context);
        sys_free(item);
        item = static_cast<work_item_t*>(m_pWorkQueue->TryDequeue());
        ++count;
    }

    if (count > m_pWorkQueue->Capacity())
        LOG_DEBUG(LOG_TAG, "%s growing event queue on shutdown.", __func__);

    LOG_WARN(LOG_TAG, "%s: thread id %d, thread name %s exited", __func__,
           m_tid, m_name);
    
    return NULL;
}

void* Thread::run_thread(void* arg) {
    CHECK(arg != NULL);
    struct entry_arg* entry = static_cast<entry_arg*>(arg);    
    return entry->thread->Run(arg);
}


static void work_queue_read_cb(void* context) {
  CHECK(context != NULL);

  FixedQueue* queue = (FixedQueue*)context;
  work_item_t* item = static_cast<work_item_t*>(queue->Dequeue());
  item->func(item->context);
  sys_free(item);
}
