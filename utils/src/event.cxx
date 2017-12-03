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

#define LOG_TAG "event_proxy"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include "utils/inc/event.h"

#if !defined(EFD_SEMAPHORE)
#define EFD_SEMAPHORE (1 << 0)
#endif


Event::Event()
    :m_fd(INVALID_FD)
{
    New();
}

Event::~Event() {
    Free();
}

void Event::Wait() {
    eventfd_t value;
    if (eventfd_read(m_fd, &value) == -1)
        LOG_ERROR(LOG_TAG, "%s unable to wait on event: %s", __func__,
              strerror(errno));
}

bool Event::TryWait() {
    int flags = fcntl(m_fd, F_GETFL);
    if (-1 == flags) {
        LOG_ERROR(LOG_TAG, ""%s unable to get flags for event fd: %s",
              __func__, strerror(errno));
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR(LOG_TAG, "%s unable to set O_NONBLOCK for event fd: %s",
              __func__, strerror(errno));
        return false;
    }
    bool ret = true;
    eventfd_t value;
    if (eventfd_read(m_fd, &value) == -1) ret = false;

    if (fcntl(m_fd, F_SETFL, flags) == -1)
    LOG_ERROR(LOG_TAG, "%s unable to restore flags for event fd: %s",
          __func__, strerror(errno));
    return ret;  
}

void Event::Post() {
    if (-1 == eventfd_write(event->fd, 1ULL))
        LOG_ERROR(LOG_TAG, "%s unable to wait on event: %s", __func__,
            strerror(errno));
}

void Event::New() {
    CHECK(m_fd == INVALID_FD);
    m_fd = eventfd(value, EFD_SEMAPHORE);
}

void Event::Free() {
    if (INVALID_FD != m_fd) close(m_fd);
    m_fd = INVALID_FD;
}
