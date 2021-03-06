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

#include "utils.h"
#include "allocator.h"
#include "seqlist.h"
#include "reactor.h"

struct reactor_object_t {
  int fd;              // the file descriptor to monitor for events.
  void* context;       // a context that's passed back to the *_ready functions.
  Reactor* reactor;  // the reactor instance this object is registered with.
  std::mutex* mutex;  // protects the lifetime of this object and all variables.
  void (*read_ready)(void* context);   // function to call when the file descriptor becomes readable.
  void (*write_ready)(void* context);  // function to call when the file descriptor becomes writeable.
};

static const size_t MAX_EVENTS = 64;
static const eventfd_t EVENT_REACTOR_STOP = 1;

Reactor::Reactor()
   :m_epollFd(INVALID_FD)
   ,m_eventFd(INVALID_FD)
   ,m_isRunning(false)
   ,m_isObjectRemoved(false)
   ,m_invalidList(NULL)
{
    New();
}

Reactor::~Reactor() {
    Free();
}

void Reactor::New() {
    m_epollFd = epoll_create(MAX_EVENTS);
    m_eventFd = eventfd(0, 0); 
    m_invalidList = new SeqList(NULL);
    
    CHECK(m_epollFd != INVALID_FD);
    CHECK(m_eventFd != INVALID_FD);
    CHECK(m_invalidList != NULL);
    
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    if (-1 == epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &event)) {
        LOG_ERROR(LOG_TAG, "unable to register eventfd with epoll set: %s", strerror(errno));
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
    if (m_invalidList != NULL) {
        delete m_invalidList;
        m_invalidList = NULL;
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
        LOG_ERROR(LOG_TAG, "unable to register fd %d to epoll set: %s",
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

    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, obj->fd, NULL) == -1)
        LOG_ERROR(LOG_TAG, "unable to unregister fd %d from epoll set: %s",
                  obj->fd, strerror(errno));

    if (m_isRunning &&
        pthread_equal(pthread_self(), reactor->m_runThread)) {
        reactor->m_isObjectRemoved = true;
        return;
    }
    
	reactor->m_invalidList->Append(obj);


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
    sys_free(obj);
}

// Runs the reactor loop for a maximum of |iterations|.
// 0 |iterations| means loop forever.
// |reactor| may not be NULL.
reactor_status_t Reactor::Run(int iterations) {

    m_runThread = pthread_self();
    m_isRunning = true;

    struct epoll_event events[MAX_EVENTS];
    for (int i = 0; iterations == 0 || i < iterations; ++i) {
        if (m_invalidList != NULL) m_invalidList->Clear();

        int ret;
		SYS_NO_INTR(ret = epoll_wait(m_epollFd, events, MAX_EVENTS, -1));
        if (ret == -1) {
            LOG_ERROR(LOG_TAG, "error in epoll_wait: %s", strerror(errno));
            m_isRunning = false;
            return REACTOR_STATUS_ERROR;
        }

        for (int j = 0; j < ret; ++j) {
            // The event file descriptor is the only one that registers with
            // a NULL data pointer. We use the NULL to identify it and break
            // out of the reactor loop.
            if (events[j].data.ptr == NULL) {
                eventfd_t value;
                eventfd_read(m_eventFd, &value);
                m_isRunning = false;
                return REACTOR_STATUS_STOP;
            }

            reactor_object_t* object = (reactor_object_t*)events[j].data.ptr;
            Reactor* reactor = object->reactor;
            if (m_invalidList->Contains(object)) continue;    

            // Downgrade the list lock to an object lock.
            {
                std::lock_guard<std::mutex> obj_lock(*object->mutex);

                reactor->m_isObjectRemoved = false;
                if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) &&
                        object->read_ready)
                    object->read_ready(object->context);
                if (!reactor->m_isObjectRemoved && events[j].events & EPOLLOUT &&
                        object->write_ready)
                    object->write_ready(object->context);
            }

            if (reactor->m_isObjectRemoved) {
                delete object->mutex;
                sys_free(object);
            }
        }
    }

    m_isObjectRemoved = false;
    return REACTOR_STATUS_DONE;
}

