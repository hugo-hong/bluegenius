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
#define LOG_TAG "bluegenius_utils_future"

#include "utils.h"
#include "eventlock.h"
#include "future.h"

Future::Future(void *value)
	:m_ready(false)
	,m_result(NULL)
	,m_event(NULL)
{
	New(value);
}

Future::~Future() {
	Free();
}

void Future::Ready(void *value) {
	CHECK(m_event != NULL);
	CHECK(m_ready != false);
	m_result = value;
	m_event->Post();
}

void* Future::Await() {
	if (m_event != NULL)
		m_event->Wait();

	void *result = m_result;
	Free();
	return result;
}

void Future::New(void *value) {
	m_event = new EventLock(0);
	m_ready = true;
	m_result = value;
}

void Future::Free() {
	if (m_event != NULL)
		delete m_event;
	m_ready = false;
}


