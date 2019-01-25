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

#define LOG_TAG "utils_eventlock"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include "utils.h"
#include "eventlock.h"

#ifndef EFD_SEMAPHORE
#define EFD_SEMAPHORE (1 << 0)
#endif


EventLock::EventLock(int value)
    :m_fd(INVALID_FD)
{
    New(value);
}

EventLock::~EventLock() {
    Free();
}

int EventLock::Wait() {
    eventfd_t value;
	return eventfd_read(m_fd, &value);    
}

bool EventLock::TryWait() {
    int flags = fcntl(m_fd, F_GETFL);
    if (-1 == flags) {
        LOG_ERROR(LOG_TAG, "unable to get flags for event fd: %s", strerror(errno));
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR(LOG_TAG, "unable to set O_NONBLOCK for event fd: %s", strerror(errno));
        return false;
    }
    bool ret = true;
    eventfd_t value;
    if (eventfd_read(m_fd, &value) == -1) ret = false;

    if (fcntl(m_fd, F_SETFL, flags) == -1)
		LOG_ERROR(LOG_TAG, "%s unable to restore flags for event fd: %s", strerror(errno));

    return ret;  
}

int EventLock::Post() {
	return eventfd_write(m_fd, 1ULL);   
}

void EventLock::New(int value) {
    CHECK(m_fd == INVALID_FD);
    m_fd = eventfd(value, EFD_SEMAPHORE);
}

void EventLock::Free() {
    if (INVALID_FD != m_fd) 
		close(m_fd);
    m_fd = INVALID_FD;
}
