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
