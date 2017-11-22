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

static const size_t DEFAULT_WORK_QUEUE_CAPACITY = 128;

struct thread_t {
  std::atomic_bool is_joined{false};
  pthread_t pthread;
  pid_t tid;
  char name[THREAD_NAME_MAX + 1];
  reactor_t* reactor;
  fixed_queue_t* work_queue;
};

struct entry_arg {
  thread_t* thread;
  event_t* entry_evt;
  int error;
};

typedef struct {
  thread_fn func;
  void* context;
} work_item_t;

thread_t* thread_new(const char* name) {
   CHECK(name != NULL);
   
   thread_t* thread = static_cast<thread_t*>(sys_calloc(sizeof(thread_t)));
   thread->reactor = reactor_new();
   if (NULL == thread->reactor) goto error;
   
   thread->work_queue = fixed_queue_new(DEFAULT_WORK_QUEUE_CAPACITY);
   if (NULL == thread->work_queue) goto error;
   
   // Start is on the stack, but we use a semaphore, so it's safe
  struct entry_arg arg;
  arg.entry_evt = event_new(0);
  if (NULL == arg.entry_evt) goto error;

  strncpy(thread->name, name, THREAD_NAME_MAX);
  arg.thread = thread;
  arg.error = 0;
  pthread_create(&ret->pthread, NULL, run_thread, &arg);
  event_wait(arg.entry_evt);
  event_free(arg.entry_evt);

  if (entry.error) goto error;

  return thread;

error:
  if (thread) {
    fixed_queue_free(ret->work_queue, osi_free);
    reactor_free(ret->reactor);
  }
  sys_free(thread);
  return NULL;
}

void thread_free(thread_t* thread) {
  if (!thread) return;

  thread_stop(thread);
  thread_join(thread);

  fixed_queue_free(thread->work_queue, sys_free);
  reactor_free(thread->reactor);
  sys_free(thread);
}

void thread_join(thread_t* thread) {
  CHECK(thread != NULL);

  if (!std::atomic_exchange(&thread->is_joined, true))
    pthread_join(thread->pthread, NULL);
}

bool thread_post(thread_t* thread, thread_fn func, void* context) {
  CHECK(thread != NULL);
  CHECK(func != NULL);

  // TODO(sharvil): if the current thread == |thread| and we've run out
  // of queue space, we should abort this operation, otherwise we'll
  // deadlock.

  // Queue item is freed either when the queue itself is destroyed
  // or when the item is removed from the queue for dispatch.
  work_item_t* item = (work_item_t*)sys_malloc(sizeof(work_item_t));
  item->func = func;
  item->context = context;
  fixed_queue_enqueue(thread->work_queue, item);
  return true;
}

void thread_stop(thread_t* thread) {
  CHECK(thread != NULL);
  reactor_stop(thread->reactor);
}

bool thread_set_priority(thread_t* thread, int priority) {
  if (!thread) return false;

  const int rc = setpriority(PRIO_PROCESS, thread->tid, priority);
  if (rc < 0) {
    LOG_ERROR(LOG_TAG,
              "%s unable to set thread priority %d for tid %d, error %d",
              __func__, priority, thread->tid, rc);
    return false;
  }

  return true;
}

bool thread_set_rt_priority(thread_t* thread, int priority) {
  if (!thread) return false;

  struct sched_param rt_params;
  rt_params.sched_priority = priority;

  const int rc = sched_setscheduler(thread->tid, SCHED_FIFO, &rt_params);
  if (rc != 0) {
    LOG_ERROR(LOG_TAG,
              "%s unable to set SCHED_FIFO priority %d for tid %d, error %s",
              __func__, priority, thread->tid, strerror(errno));
    return false;
  }

  return true;
}

bool thread_is_self(const thread_t* thread) {
  CHECK(thread != NULL);
  return !!pthread_equal(pthread_self(), thread->pthread);
}

reactor_t* thread_get_reactor(const thread_t* thread) {
  CHECK(thread != NULL);
  return thread->reactor;
}

const char* thread_name(const thread_t* thread) {
  CHECK(thread != NULL);
  return thread->name;
}

static void* run_thread(void* arg) {
  CHECK(arg != NULL);

  struct entry_arg* entry = static_cast<struct entry_arg*>(arg);
  thread_t* thread = entry->thread;

  CHECK(thread != NULL);

  if (prctl(PR_SET_NAME, (unsigned long)thread->name) == -1) {
    LOG_ERROR(LOG_TAG, "%s unable to set thread name: %s", __func__,
              strerror(errno));
    entry->error = errno;
    event_post(entry->entry_evt);
    return NULL;
  }
  thread->tid = gettid();

  LOG_INFO(LOG_TAG, "%s: thread id %d, thread name %s started", __func__,
           thread->tid, thread->name);

  event_post(entry->entry_evt);

  int fd = fixed_queue_get_dequeue_fd(thread->work_queue);
  void* context = thread->work_queue;

  reactor_object_t* work_queue_object =
      reactor_register(thread->reactor, fd, context, work_queue_read_cb, NULL);
  reactor_start(thread->reactor);
  reactor_unregister(work_queue_object);

  // Make sure we dispatch all queued work items before exiting the thread.
  // This allows a caller to safely tear down by enqueuing a teardown
  // work item and then joining the thread.
  size_t count = 0;
  work_item_t* item =
      static_cast<work_item_t*>(fixed_queue_try_dequeue(thread->work_queue));
  while (item && count <= fixed_queue_capacity(thread->work_queue)) {
    item->func(item->context);
    osi_free(item);
    item =
        static_cast<work_item_t*>(fixed_queue_try_dequeue(thread->work_queue));
    ++count;
  }

  if (count > fixed_queue_capacity(thread->work_queue))
    LOG_DEBUG(LOG_TAG, "%s growing event queue on shutdown.", __func__);

  LOG_WARN(LOG_TAG, "%s: thread id %d, thread name %s exited", __func__,
           thread->tid, thread->name);
  return NULL;
}
