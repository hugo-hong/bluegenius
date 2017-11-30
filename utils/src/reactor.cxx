/********************************************************************************
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
#define LOG_TAG "utils_reactor"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <mutex>

#include "utils/inc/list.h"
#include "utils/inc/reactor.h"

struct reactor_t {
  int epoll_fd;
  int event_fd;
  std::mutex* list_mutex;
  list_t* invalidation_list;  // reactor objects that have been unregistered.
  pthread_t run_thread;       // the pthread on which reactor_run is executing.
  bool is_running;            // indicates whether |run_thread| is valid.
  bool object_removed;
}

struct reactor_object_t {
  int fd;              // the file descriptor to monitor for events.
  void* context;       // a context that's passed back to the *_ready functions.
  Reactor* reactor;  // the reactor instance this object is registered with.
  std::mutex* mutex;  // protects the lifetime of this object and all variables.
  void (*read_ready)(void* context);   // function to call when the file descriptor becomes readable.
  void (*write_ready)(void* context);  // function to call when the file descriptor becomes writeable.
}

static const size_t MAX_EVENTS = 64;
static const eventfd_t EVENT_REACTOR_STOP = 1;


Reactor::Reactor()
   :m_epollFd(INVALID_FD)
   ,m_eventFd(INVALID_FD)
{
    New();
}

Reactor::~Reactor() {
    Free();
}

void Reactor::New() {
    m_epollFd = epoll_create(MAX_EVENTS);
    m_eventFd = eventfd(0, 0); 
    
    CHECK(m_epollFd != INVALID_FD);
    CHECK(m_eventFd != INVALID_FD);
    
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    if (-1 == epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &event)) {
        LOG_ERROR(LOG_TAG, "%s unable to register eventfd with epoll set: %s",
            __func__, strerror(errno));
        Free();
        CHECK(0);
    }
}

void Reactor::Free() {
    if (m_epollFd != INVALID_FD) {
        close(m_epollFd);
        m_epollFd = INVALID_FD;
    }
    if (m_eventFd != INVALID_FD) {
        close(m_eventFd);
        m_eventFd = INVALID_FD;
    }
}

reactor_status_t Reactor::Start() {
    return Run(0);
}

reactor_status_t Reactor::RunOnce() {
    return Run(1);
}

void Reactor::Stop() {
    eventfd_write(m_eventFd, EVENT_REACTOR_STOP);
}

reactor_object_t* Reactor::Register(int fd, void* context, ready_cb pfnRead, ready_cb pfnWrite) {
    reactor_object_t* object = (reactor_object_t*)sys_calloc(sizeof(reactor_object_t));
    CHECK(object != NULL);
    object->reactor = this;
    object->fd = fd;
    object->context = context;
    object->read_ready = pfnRead;
    object->write_ready = pfnWrite;
    object->mutex = new std::mutex;
    
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    if (pfnRead != NULL) event.events |= (EPOLLIN | EPOLLRDHUP);
    if (pfnWrite != NULL) event.events |= EPOLLOUT;
    event.data.ptr = object;
    
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOG_ERROR(LOG_TAG, "%s unable to register fd %d to epoll set: %s", __func__,
          fd, strerror(errno));
        delete object->mutex;
        sys_free(object);
        return NULL;
    }

    return object;    
}

void Reactor::Unregister(reactor_object_t* obj) {
    CHECK(obj != NULL);

    Reactor* reactor = obj->reactor;
    
    CHECK(this == reactor);

    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, obj->fd, NULL) == -1)
        LOG_ERROR(LOG_TAG, "%s unable to unregister fd %d from epoll set: %s",
                  __func__, obj->fd, strerror(errno));

    if (m_Isrunning &&
        pthread_equal(pthread_self(), reactor->run_thread)) {
        reactor->object_removed = true;
        return;
    }

    {
        std::unique_lock<std::mutex> lock(*reactor->list_mutex);
        list_append(reactor->invalidation_list, obj);
    }

    // Taking the object lock here makes sure a callback for |obj| isn't
    // currently executing. The reactor thread must then either be before
    // the callbacks or after. If after, we know that the object won't be
    // referenced because it has been taken out of the epoll set. If before,
    // it won't be referenced because the reactor thread will check the
    // invalidation_list and find it in there. So by taking this lock, we
    // are waiting until the reactor thread drops all references to |obj|.
    // One the wait completes, we can unlock and destroy |obj| safely.
    obj->mutex->lock();
    obj->mutex->unlock();
    delete obj->mutex;
    sis_free(obj);
}

// Runs the reactor loop for a maximum of |iterations|.
// 0 |iterations| means loop forever.
// |reactor| may not be NULL.
reactor_status_t Reactor::Run(int iterations) {

  m_runThread = pthread_self();
  m_isRunning = true;

  struct epoll_event events[MAX_EVENTS];
  for (int i = 0; iterations == 0 || i < iterations; ++i) {
    {
      std::lock_guard<std::mutex> lock(*reactor->list_mutex);
      list_clear(reactor->invalidation_list);
    }

    int ret;
    OSI_NO_INTR(ret = epoll_wait(reactor->epoll_fd, events, MAX_EVENTS, -1));
    if (ret == -1) {
      LOG_ERROR(LOG_TAG, "%s error in epoll_wait: %s", __func__,
                strerror(errno));
      m_isRunning = false;
      return REACTOR_STATUS_ERROR;
    }

    for (int j = 0; j < ret; ++j) {
      // The event file descriptor is the only one that registers with
      // a NULL data pointer. We use the NULL to identify it and break
      // out of the reactor loop.
      if (events[j].data.ptr == NULL) {
        eventfd_t value;
        eventfd_read(reactor->event_fd, &value);
        m_isRunning = false;
        return REACTOR_STATUS_STOP;
      }

      reactor_object_t* object = (reactor_object_t*)events[j].data.ptr;

      std::unique_lock<std::mutex> lock(*reactor->list_mutex);
      if (list_contains(reactor->invalidation_list, object)) {
        continue;
      }

      // Downgrade the list lock to an object lock.
      {
        std::lock_guard<std::mutex> obj_lock(*object->mutex);
        lock.unlock();

        reactor->object_removed = false;
        if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) &&
            object->read_ready)
          object->read_ready(object->context);
        if (!reactor->object_removed && events[j].events & EPOLLOUT &&
            object->write_ready)
          object->write_ready(object->context);
      }

      if (reactor->object_removed) {
        delete object->mutex;
        osi_free(object);
      }
    }
  }

  reactor->is_running = false;
  return REACTOR_STATUS_DONE;
}

